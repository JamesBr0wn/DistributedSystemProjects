//
// Created by JamesBrown on 2018/12/26.
//

#include "Client.h"

Status ClientImp::getBlock(const BlockInfo request){
    BlockUnit unit;
    ClientContext context;
    std::unique_ptr<ClientReader<BlockUnit>> reader(
            dataStub->getBlock(&context, request));
    Status status;

    string blockName = request.filename() + '.' + to_string(request.blockidx());
    cout << "$ Get block " << blockName << "." << endl;
    ofstream ofs(cacheDirectory + blockName, ios::out | ios::binary | ios::ate);
    if(!ofs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl;
        return Status::CANCELLED;
    }

    unsigned long i = 0;
    while (reader->Read(&unit)) {
        ofs.write(unit.unitdata().data(), unit.unitdata().length());
        i++;
    }
    ofs.close();
    status = reader->Finish();

    if (status.ok()) {
        cout << "$ Block get finished!" << endl;
        return Status::OK;
    } else {
        cout << "# Block " << blockName << " get failed!" << endl;
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
    string blockName = fileName + '.' + to_string(blockIdx);
    cout << "$ Put block " << blockName << "." << endl;
    ifstream ifs(cacheDirectory + blockName, ios::in | ios::binary | ios::ate);
    if(!ifs.is_open()){
        cout << "# Block " << blockName << " open failed!" << endl;
        return Status::CANCELLED;
    }
    blockSize = ifs.tellg();

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
        writer->Write(unit);
    }

    delete[] unitData;
    ifs.close();
    writer->WritesDone();
    status = writer->Finish();

    if (status.ok()) {
        // 检查blockSize是否相符
        if(blockSize != reply.blocksize()){
            cout << "# Block " << blockName << " size mismatch!" << endl;
            return Status::CANCELLED;
        }

        // 验证Hash值
        SHA256_Final(hash, &stx);
        if(memcmp(hash, reply.blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
            cout << "Local hash ";
            for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
                cout << (int)(hash[i]) << " ";
            }
            cout << endl;
            cout << "Remote hash ";
            for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
                cout << (int)(reply.blockhash().data()[i]) << " ";
            }
            cout << endl;
            cout << "# Block " << blockName << " hash mismatch!" << endl;
            return Status::CANCELLED;
        }

        cout << "$ Block put finished!" << endl;
        return Status::OK;
    } else {
        cout << "# Block " << blockName << " put failed!" << endl;
        return Status::CANCELLED;
    }
}

Status ClientImp::rmBlock(const BlockInfo request){
    BlockInfo reply;
    ClientContext context;
    Status status;
    string blockName = request.filename() + '.' + to_string(request.blockidx());
    cout << "$ Rm block " << blockName << "." << endl;
    status = dataStub->rmBlock(&context, request, &reply);
    if(status.ok()){
        cout << "$ Block rm finished!" << endl;
    }else{
        cout << "$ Block " << blockName << " rm failed!" << endl;
    }
    return status;
}

Status ClientImp::get(const string fileName){
    cout << "$ Get file " << fileName << "." << endl;
    FileInfo fileInfo;
    fileInfo.set_filename(fileName);
    BlockStore blockStore;
    ClientContext start;
    std::unique_ptr<ClientReader<BlockStore>> blockStoreReader(
            nameStub->beginGetTransaction(&start, fileInfo));
    Status status;
    vector<BlockStore> storeList;
    while (blockStoreReader->Read(&blockStore)) {
        storeList.push_back(blockStore);
    }
    status = blockStoreReader->Finish();
    if(!status.ok()){
        cout << "# File " << fileInfo.filename() << " get block list fail!" << endl;
    }

    BlockInfo tempInfo;
    long lastBlockIdx = -1;
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        if(it->blockidx() == lastBlockIdx){
            continue;
        }else if(it->blockidx() > lastBlockIdx + 1){
            ClientContext abort;
            nameStub->abortGetTransaction(&abort, fileInfo, &fileInfo);
            return Status::CANCELLED;
        }
        dataStub = DataService::NewStub(grpc::CreateChannel(it->serveraddress(), grpc::InsecureChannelCredentials()));
        tempInfo.set_filename(it->filename());
        tempInfo.set_blockidx(it->blockidx());
        tempInfo.set_blocksize(it->blocksize());
        tempInfo.set_blockhash(it->blockhash());
        if(getBlock(tempInfo).ok()){
            lastBlockIdx++;
        }
    }

    ifstream bfs;
    ofstream ffs;
    unsigned long blockSize;
    char tempUnit[unitSize];
    lastBlockIdx = -1;
    ffs.open(cacheDirectory + fileName, ios::out | ios::binary | ios::ate);
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        if(it->blockidx() == lastBlockIdx){
            continue;
        }
        bfs.open(cacheDirectory + it->filename() + "." + to_string(it->blockidx()), ios::in | ios::binary);
        blockSize = it->blocksize();
        for(unsigned long i = 0; i < blockSize / unitSize; i++){
            bfs.read(tempUnit, unitSize);
            ffs.write(tempUnit, unitSize);
        }
        if(blockSize % unitSize != 0){
            bfs.read(tempUnit, blockSize % unitSize);
            ffs.write(tempUnit, blockSize % unitSize);
        }
        bfs.close();
        if(remove((cacheDirectory + it->filename() + "." + to_string(it->blockidx())).data())){
            cout << "# Cached block " << cacheDirectory + it->filename() + '.' + to_string(it->blockidx()) << " remove fail!" << endl;
            ClientContext abort;
            nameStub->abortGetTransaction(&abort, fileInfo, &fileInfo);
            return Status::CANCELLED;
        }
        lastBlockIdx++;
    }
    ffs.close();
    ClientContext commit;
    nameStub->commitGetTransaction(&commit, fileInfo, &fileInfo);
    cout << "$ File get finished!" << endl;
    return Status::OK;
}

Status ClientImp::put(const string fileName){
    cout << "$ Put file " << fileName << "." << endl;
    FileInfo fileInfo;
    fileInfo.set_filename(fileName);
    ifstream tempIfs(cacheDirectory + fileName, ios::out | ios::binary | ios::app);
    tempIfs.seekg(0,ifstream::end);
    fileInfo.set_filesize(tempIfs.tellg());
    tempIfs.close();
    BlockStore blockStore;
    ClientContext start;
    std::unique_ptr<ClientReader<BlockStore>> blockStoreReader(
            nameStub->beginPutTransaction(&start, fileInfo));
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

    ifstream ffs;
    ofstream bfs;
    unsigned long blockSize;
    long lastBlockIdx = -1;
    char tempUnit[unitSize];
    ffs.open(cacheDirectory + fileName, ios::in | ios::binary);
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        if(it->blockidx() == lastBlockIdx){
            continue;
        }
        bfs.open(cacheDirectory + it->filename() + "." + to_string(it->blockidx()), ios::out | ios::binary | ios::ate);
        blockSize = it->blocksize();
        for(i = 0; i < blockSize / unitSize; i++){
            ffs.read(tempUnit, unitSize);
            bfs.write(tempUnit, unitSize);
        }
        if(blockSize % unitSize != 0){
            ffs.read(tempUnit, blockSize % unitSize);
            bfs.write(tempUnit, blockSize % unitSize);
        }
        bfs.close();
        lastBlockIdx++;
    }
    ffs.close();

    BlockInfo tempInfo;
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        tempInfo.set_filename(fileName);
        tempInfo.set_blockidx(it->blockidx());
        tempInfo.set_blocksize(it->blocksize());
        dataStub = DataService::NewStub(grpc::CreateChannel(it->serveraddress(), grpc::InsecureChannelCredentials()));
        if(!putBlock(tempInfo).ok()){
            ClientContext abort;
            nameStub->abortPutTransaction(&abort, fileInfo, &fileInfo);
            return Status::CANCELLED;
        }
    }

    lastBlockIdx = -1;
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        if(it->blockidx() == lastBlockIdx){
            continue;
        }
        if(remove((cacheDirectory + it->filename() + "." + to_string(it->blockidx())).data())){
            cout << "# Cached block " << cacheDirectory + it->filename() + to_string(it->blockidx()) << " remove fail!" << endl;
            ClientContext abort;
            nameStub->abortPutTransaction(&abort, fileInfo, &fileInfo);
            return Status::CANCELLED;
        }
        lastBlockIdx++;
    }

    ClientContext commit;
    nameStub->commitPutTransaction(&commit, fileInfo, &fileInfo);
    cout << "# File put finished!" << endl;
    return Status::OK;
}

Status ClientImp::rm(string fileName) {
    cout << "$ Rm file " << fileName << "." << endl;
    FileInfo fileInfo;
    fileInfo.set_filename(fileName);
    BlockStore blockStore;
    ClientContext start;
    std::unique_ptr<ClientReader<BlockStore>> blockStoreReader(
            nameStub->beginRmTransaction(&start, fileInfo));
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

    BlockInfo tempInfo;
    for(auto it = storeList.begin(); it != storeList.end(); it++){
        dataStub = DataService::NewStub(grpc::CreateChannel(it->serveraddress(), grpc::InsecureChannelCredentials()));
        tempInfo.set_filename(it->filename());
        tempInfo.set_blockidx(it->blockidx());
        tempInfo.set_blocksize(it->blocksize());
        tempInfo.set_blockhash(it->blockhash());
        if(!rmBlock(tempInfo).ok()){
            ClientContext abort;
            nameStub->abortRmTransaction(&abort, fileInfo, &fileInfo);
            return Status::CANCELLED;
        }
    }

    if(remove((cacheDirectory + fileName).data())){
        cout << "# File " << fileName << " remove fail!" << endl;
        ClientContext abort;
        nameStub->abortRmTransaction(&abort, fileInfo, &fileInfo);
        return Status::CANCELLED;
    }

    ClientContext commit;
    nameStub->commitRmTransaction(&commit, fileInfo, &fileInfo);
    cout << "$ File rm finished!" << endl;
    return Status::OK;
}
