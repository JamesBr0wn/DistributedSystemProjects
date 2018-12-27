//
// Created by JamesBrown on 2018/12/26.
//

#include <NameServer.pb.h>
#include "Client.h"

Status ClientImp::getBlock(const BlockInfo request){
    BlockUnit unit;
    ClientContext context;
    std::unique_ptr<ClientReader<BlockUnit>> reader(
            dataStub->getBlock(&context, request));
    Status status;

    string blockName = request.filename() + '_' + to_string(request.blockidx());
    ofstream ofs(cacheDirectory + blockName, ios::out | ios::binary | ios::ate);
    if(!ofs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl << endl;
        return Status::CANCELLED;
    }

    unsigned long i = 0;
    while (reader->Read(&unit)) {
        ofs.write(unit.unitdata().data(), unit.unitdata().length());
        cout << "$ Block " << blockName << " get unit " << i << endl;
        i++;
    }
    ofs.close();
    status = reader->Finish();

    if (status.ok()) {
        cout << "$ Block " << blockName << " get finished!" << endl << endl;
        return Status::OK;
    } else {
        cout << "# Block " << blockName << " get failed!" << endl << endl;
        return Status::CANCELLED;
    }
}

Status ClientImp::putBlock(const BlockInfo request){
    BlockInfo reply;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientWriter<BlockUnit>> writer(dataStub->putBlock(&context, &reply));

    string fileName = request.filename();
    unsigned long blockIdx = request.blockidx();
    long blockSize;
    string blockName = fileName + '_' + to_string(blockIdx);
    ifstream ifs(cacheDirectory + blockName, ios::in | ios::binary | ios::ate);
    if(!ifs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl << endl;
        return Status::CANCELLED;
    }
    blockSize = ifs.tellg();
    cout << "$ Block " << blockName << " size: " << blockSize << endl;

    // 初始化流传输相关变量
    ifs.seekg(0, ios::beg);
    unsigned long unitNum = blockSize / unitSize;
    char* unitData = new char[unitSize];
    BlockUnit unit;

    // 初始化Hash检测相关变量
    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);

    // 读取Block并传输
    for(unsigned long i = 0; i < unitNum; i++){
        ifs.read(unitData, unitSize);
        SHA256_Update(&stx, unitData, (size_t)unitSize);
        unit.set_filename(fileName);
        unit.set_blockidx(blockIdx);
        unit.set_unitidx(i);
        unit.set_unitdata(unitData, (size_t)unitSize);
        unit.set_lastunit(blockSize % unitSize == 0 && i == unitNum - 1);
        cout << "$ Block " << blockName << " put unit " << i << endl;
        writer->Write(unit);
    }
    if(blockSize % unitSize != 0){
        ifs.read(unitData, blockSize % unitSize);
        SHA256_Update(&stx, unitData, (size_t)(blockSize % unitSize));
        unit.set_filename(fileName);
        unit.set_blockidx(blockIdx);
        unit.set_unitidx(unitNum);
        unit.set_unitdata(unitData, (size_t)blockSize % unitSize);
        unit.set_lastunit(true);
        cout << "$ Block " << blockName << " put unit " << unitNum << endl;
        writer->Write(unit);
    }

    delete[] unitData;
    ifs.close();
    writer->WritesDone();
    status = writer->Finish();

    if (status.ok()) {
        // 检查blockSize是否相符
        if(blockSize != reply.blocksize()){
            cout << "# Block " << blockName << " size mismatch!" << endl << endl;
            return Status::CANCELLED;
        }

        // 验证Hash值
        SHA256_Final(hash, &stx);
        cout << "$ Block " << blockName << " hash: ";
        cout << hex;
        for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
            cout << (unsigned int)hash[i];
        }
        cout << dec << endl;
        if(memcmp(hash, reply.blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
            cout << "# Block " << blockName << " hash mismatch!" << endl << endl;
            return Status::CANCELLED;
        }

        cout << "$ Block " << blockName << " put finished!" << endl << endl;
        return Status::CANCELLED;
    } else {
        cout << "# Block " << blockName << " put failed!" << endl << endl;
        return Status::CANCELLED;
    }
}

Status ClientImp::rmBlock(const BlockInfo request){
    BlockInfo reply;
    ClientContext context;
    Status status;
    string blockName = request.filename() + '_' + to_string(request.blockidx());

    status = dataStub->rmBlock(&context, request, &reply);
    if(status.ok()){
        cout << "$ Block " << blockName << " rm finished!" << endl << endl;
    }else{
        cout << "$ Block " << blockName << " rm failed!" << endl << endl;
    }
    return status;
}

Status ClientImp::get(const string fileName){
    FileInfo fileInfo;
    fileInfo.set_filename(fileName);
    BlockStore blockStore;
    ClientContext context;
    std::unique_ptr<ClientReader<BlockStore>> blockStoreReader(
            nameStub->beginGetTransaction(&context, fileInfo));
    Status status;
    unsigned long i = 0;
    vector<BlockStore> storeList;
    while (blockStoreReader->Read(&blockStore)) {
        storeList.push_back(blockStore);
        i++;
    }
    status = blockStoreReader->Finish();
    if(!status.ok()){
        cout << "# File " << fileInfo.filename() << " get block list fail!" << endl;
    }

    BlockInfo temp;
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        dataStub = DataService::NewStub(grpc::CreateChannel(it->serverinfo().address(), grpc::InsecureChannelCredentials()));
        temp.set_filename(it->blockinfo().filename());
        temp.set_blockidx(it->blockinfo().blockidx());
        temp.set_blocksize(it->blockinfo().blocksize());
        temp.set_blockhash(it->blockinfo().blockhash());
        if(!getBlock(temp).ok()){
            return Status::CANCELLED;
        }
    }
    return Status::OK;
}

class TestClient {
public:
    TestClient(string dir, size_t sz) : clientImp(grpc::CreateChannel(std::string(SERVER_ADDRESS), grpc::InsecureChannelCredentials()), dir, sz){}

    bool getBlockTest(){
        BlockInfo request;
        request.set_filename("block");
        request.set_blockidx(0);
        string blockName = request.filename() + '_' + to_string(request.blockidx());
        ifstream ifs("Temp/" + blockName, ios::in | ios::binary | ios::ate);
        long blockSize = ifs.tellg();
        cout << "Block " << blockName << " size: " << blockSize << endl;
        request.set_blocksize(blockSize);
        ifs.seekg(0, ios::beg);
        char * blockData = new char[blockSize];
        ifs.read(blockData, blockSize);

        SHA256_CTX stx;
        unsigned char hash[SHA256_DIGEST_LENGTH+1];
        hash[SHA256_DIGEST_LENGTH] = '\0';
        SHA256_Init(&stx);
        SHA256_Update(&stx, blockData, (size_t)blockSize);
        SHA256_Final(hash, &stx);
        cout << "Block " << blockName << " hash: ";
        cout << hex;
        for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
            cout << (unsigned int)hash[i];
        }
        cout << dec << endl;
        request.set_blockhash((char*)hash);
        delete[] blockData;
        ifs.close();

        clientImp.getBlock(request);
        return true;
    }

    bool putBlockTest(){
        BlockInfo request;
        request.set_filename("block");
        request.set_blockidx(0);
        clientImp.putBlock(request);
        return true;
    }

    bool rmBlockTest(){
        BlockInfo request;
        request.set_filename("block");
        request.set_blockidx(0);
        string blockName = request.filename() + '_' + to_string(request.blockidx());
        ifstream ifs("BlockStore/" + blockName, ios::in | ios::binary | ios::ate);
        long blockSize = ifs.tellg();
        cout << "Block " << blockName << " size: " << blockSize << endl;
        request.set_blocksize(blockSize);
        ifs.seekg(0, ios::beg);
        char * blockData = new char[blockSize];
        ifs.read(blockData, blockSize);

        SHA256_CTX stx;
        unsigned char hash[SHA256_DIGEST_LENGTH+1];
        hash[SHA256_DIGEST_LENGTH] = '\0';
        SHA256_Init(&stx);
        SHA256_Update(&stx, blockData, (size_t)blockSize);
        SHA256_Final(hash, &stx);
        cout << "Block " << blockName << " hash: ";
        cout << hex;
        for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
            cout << (unsigned int)hash[i];
        }
        cout << dec << endl;
        request.set_blockhash((char*)hash);
        delete[] blockData;
        ifs.close();

        clientImp.rmBlock(request);
        return true;
    }

    bool getTest(){
        clientImp.get("file");
        return true;
    }
private:
    ClientImp clientImp;
};

int main(int argc, char** argv){
    strcpy(SERVER_ADDRESS, argv[1]);
    TestClient client("BlockCache/", 1024 * 1024);
    char temp;
//    if(client.getBlockTest()){
//        std::cout << "$ Get succeed!" << std::endl;
//    }else{
//        std::cout << "# Get failed!" << std::endl;
//    }
//    cin >> temp;
//
//    if(client.rmBlockTest()){
//        std::cout << "$ Rm succeed!" << std::endl;
//    }else{
//        std::cout << "# Rm failed!" << std::endl;
//    }
//    cin >> temp;
//
//    if(client.putBlockTest()){
//        std::cout << "$ Put succeed!" << std::endl;
//    }else{
//        std::cout << "# Put failed!" << std::endl;
//    }

    if(client.getTest()){
        std::cout << "$ Get succeed!" << std::endl;
    }else{
        std::cout << "# Get failed!" << std::endl;
    }
    return 0;
}
