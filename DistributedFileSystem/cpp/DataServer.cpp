//
// Created by JamesBrown on 2018/12/26.
//

#include "DataServer.h"

char DATA_ADDRESS[32];
char NAME_ADDRESS[32];
char STORE_DIR[64];

DataServerImp::DataServerImp(std::shared_ptr<Channel> channel, string dir, size_t sz)
    : _stub(NameService::NewStub(channel)), storeDirectory(dir), unitSize(sz) {
    ServerInfo request;
    ServerInfo reply;
    ClientContext context;
    Status status;
    request.set_address(DATA_ADDRESS);
    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);
    SHA256_Update(&stx, request.address().data(), request.address().length());
    SHA256_Final(hash, &stx);
    request.set_hash(hash, SHA256_DIGEST_LENGTH);
    status = _stub->startServer(&context, request, &reply);
    if (status.ok()) {
        cout << "# Data server started!" << endl;
    } else {
        cout << "# Data server start fail!" << endl;
        exit(-1);
    }
}

DataServerImp::~DataServerImp() {
    ServerInfo request;
    ServerInfo reply;
    ClientContext context;
    Status status;
    request.set_address(DATA_ADDRESS);
    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);
    string address = request.address();
    SHA256_Update(&stx, address.data(), request.address().length());
    SHA256_Final(hash, &stx);
    request.set_hash(hash, SHA256_DIGEST_LENGTH);
    status = _stub->terminateServer(&context, request, &reply);
    if (status.ok()) {
        cout << "# Data server terminated!" << endl;
    } else {
        cout << "# Data server terminate fail!" << endl;
        exit(-1);
    }
}

Status DataServerImp::getBlock(ServerContext* context,const BlockInfo* request, ServerWriter<BlockUnit>* replyWriter) {
    // 打开Block文件
    string blockName = request->filename() + '.' + to_string(request->blockidx());
    cout << "$ Client " << context->peer() << " get block " << blockName << "." << endl;
    ifstream ifs(storeDirectory + blockName, ios::in | ios::binary | ios::ate);
    if(!ifs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl;
        return Status::CANCELLED;
    }

    // 检查blockSize是否相等
    long blockSize = ifs.tellg();
    cout << "$ Block size: " << blockSize << endl;
    if(blockSize != request->blocksize()){
        cout << "# Block " << blockName << " size mismatch!" << endl;
        return Status::CANCELLED;
    }

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
        unit.set_filename(request->filename());
        unit.set_blockidx(request->blockidx());
        unit.set_unitidx(i);
        unit.set_unitdata(unitData, (size_t)unitSize);
        unit.set_lastunit(blockSize % unitSize == 0 && i == unitNum - 1);
        replyWriter->Write(unit);
    }
    if(blockSize % unitSize != 0){
        ifs.read(unitData, blockSize % unitSize);
        SHA256_Update(&stx, unitData, (size_t)(blockSize % unitSize));
        unit.set_filename(request->filename());
        unit.set_blockidx(request->blockidx());
        unit.set_unitidx(unitNum);
        unit.set_unitdata(unitData, (size_t)blockSize % unitSize);
        unit.set_lastunit(true);
        replyWriter->Write(unit);
    }

    // 检查Hash值是否相等
    SHA256_Final(hash, &stx);
    if(memcmp(hash, request->blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
        cout << "# Block " << blockName << " hash mismatch!" << endl;
        return Status::CANCELLED;
    }

    // 完成RPC
    delete[] unitData;
    ifs.close();
    cout << "$ Block get finished!" << endl;
    return Status::OK;
}

Status DataServerImp::putBlock(ServerContext* context, ServerReader<BlockUnit>* requestReader, BlockInfo* reply) {
    // 开始流传输
    BlockUnit unit;
    unsigned long blockSize = 0;
    if(!requestReader->Read(&unit)){
        cout << "# Stream read failed!" << endl;
        return Status::CANCELLED;
    }

    // 打开Block文件
    string blockName = unit.filename() + '.' + to_string(unit.blockidx());
    cout << "$ Client " << context->peer() << " put block " << blockName << "." << endl;
    ofstream ofs(storeDirectory + blockName, ios::out | ios::binary | ios::ate);
    if(!ofs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl;
        return Status::CANCELLED;
    }
    reply->set_filename(unit.filename());
    reply->set_blockidx(unit.blockidx());

    // 初始化Hash检测相关变量
    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);

    // 写入Block并计算Hash值
    ofs.write(unit.unitdata().data(), unit.unitdata().length());
    blockSize += unit.unitdata().length();
    SHA256_Update(&stx, unit.unitdata().data(), unit.unitdata().length());
    while(requestReader->Read(&unit)){
        ofs.write(unit.unitdata().data(), unit.unitdata().length());
        blockSize += unit.unitdata().length();
        SHA256_Update(&stx, unit.unitdata().data(), unit.unitdata().length());
    }
    SHA256_Final(hash, &stx);

    reply->set_blockhash((char*)hash, SHA256_DIGEST_LENGTH);
    reply->set_blocksize(blockSize);

    // 更新Block信息
    ClientContext update;
    BlockInfo blockInfo = *reply;
    Status status = _stub->updateBlockInfo(&update, blockInfo, &blockInfo);
    if(!status.ok()){
        cout << "$ Block " << blockName << " info update fail!" << endl;
        return Status::OK;
    }
    cout << "$ Block info update finished!" << endl;

    // 完成RPC
    ofs.close();
    cout << "$ Block put finished!" << endl;
    return Status::OK;
}

Status DataServerImp::rmBlock(ServerContext* context, const BlockInfo* request, BlockInfo* reply){
    // 打开Block文件
    string blockName = request->filename() + '.' + to_string(request->blockidx());
    cout << "$ Client " << context->peer() << " rm block " << blockName << "." << endl;
    ifstream ifs(storeDirectory + blockName, ios::in | ios::binary | ios::ate);
    if(!ifs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl;
        return Status::CANCELLED;
    }

    // 检查blockSize是否相等
    long blockSize = ifs.tellg();
    cout << "$ Block " << blockName << " size: " << blockSize << endl;
    if(blockSize != request->blocksize()){
        cout << "# Block " << blockName << " size mismatch!" << endl;
        return Status::CANCELLED;
    }

    // 初始化Hash检测相关变量
    ifs.seekg(0, ios::beg);
    unsigned long unitNum = blockSize / unitSize;
    char* unitData = new char[unitSize];
    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);

    // 读取Block并计算Hash值
    for(unsigned long i = 0; i < unitNum; i++){
        ifs.read(unitData, unitSize);
        SHA256_Update(&stx, unitData, (size_t)unitSize);
    }
    if(blockSize % unitSize != 0){
        ifs.read(unitData, blockSize % unitSize);
        SHA256_Update(&stx, unitData, (size_t)(blockSize % unitSize));
    }

    // 检查Hash值是否相等
    SHA256_Final(hash, &stx);
    if(memcmp(hash, request->blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            cout << (int)(request->blockhash()[i]) << endl;
        }
        cout << endl;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            cout << (int)(hash[i]) << endl;
        }
        cout << endl;
        cout << "# Block " << blockName << " hash mismatch!" << endl;
        return Status::CANCELLED;
    }
    delete[] unitData;
    ifs.close();

    // 删除Block
    if(remove((storeDirectory + blockName).data())){
        cout << "$ Block " << blockName << " rm failed!" << endl;
        return Status::CANCELLED;
    }

    // 完成RPC
    cout << "$ Block rm finished!" << endl;
    return Status::OK;
}

void RunServer() {
    std::string server_address(DATA_ADDRESS);
    DataServerImp service(grpc::CreateChannel(
    std::string(NAME_ADDRESS), grpc::InsecureChannelCredentials()),
                          STORE_DIR, 256 * 1024);

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "# Data server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    strcpy(DATA_ADDRESS, argv[1]);
    strcpy(NAME_ADDRESS, argv[2]);
    strcpy(STORE_DIR, argv[3]);
    RunServer();

    return 0;
}