//
// Created by JamesBrown on 2018/12/26.
//

#include "NameServer.h"

char SERVER_ADDRESS[32];

int cmp(const ServerInfo& a, const ServerInfo& b){
    return memcmp(a.hash().data(), b.hash().data(), SHA256_DIGEST_LENGTH) < 0;
}

NameServerImp::NameServerImp(unsigned long size): maxBlockSize(size) {}

Status NameServerImp::startServer(ServerContext *context, const ServerInfo *request, ServerInfo *reply) {
    *reply = *request;

    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);
    string address = request->address();
    SHA256_Update(&stx, address.data(), request->address().length());
    SHA256_Final(hash, &stx);
    reply->set_hash(hash, SHA256_DIGEST_LENGTH);
    if(memcmp(hash, request->hash().data(), SHA256_DIGEST_LENGTH) != 0){
        cout << "$ Data server " << address << " hash mismatch!" << endl;
        return Status::CANCELLED;
    }

    serverList.push_back(*reply);
    sort(serverList.begin(), serverList.end(), cmp);
    cout << "$ Data server " << address << " started!" << endl;
    return Status::OK;
}

Status NameServerImp::terminateServer(ServerContext *context, const ServerInfo *request, ServerInfo *reply) {
    *reply = *request;

    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);
    string address = request->address();
    SHA256_Update(&stx, address.data(), request->address().length());
    SHA256_Final(hash, &stx);
    if(memcmp(hash, request->hash().data(), SHA256_DIGEST_LENGTH) != 0){
        cout << "$ Data server " << address << " hash mismatch!" << endl;
        return Status::CANCELLED;
    }

    for(auto iter = serverList.begin(); iter != serverList.end(); iter++){
        if(iter->address() == request->address()){
            serverList.erase(iter);
            cout << "$ Data server " << address << " terminated!" << endl;
            return Status::OK;
        }
    }
    cout << "# Data server " << address << " not found!" << endl;
    return Status::CANCELLED;
}

Status NameServerImp::beginGetTransaction(ServerContext *context, const FileInfo *request, ServerWriter<BlockStore> *replyWriter){
    string fileName = request->filename();
    cout << "$ Client " << context->peer() << " begin transaction: get " << fileName << "." << endl;
    auto fileIter = fileList.begin();
    while(fileIter != fileList.end()){
        if(fileIter->fileInfo.filename() == fileName){
            break;
        }
        fileIter++;
    }

    if(fileIter == fileList.end()){
        cout << "# File " << fileName << " not found!" << endl;
        return Status::CANCELLED;
    }

    BlockStore blockStore;
    for(long i = 0; i < fileIter->storeList.size(); i++){
        BlockInfo blockInfo = fileIter->storeList[i].first;
        ServerInfo serverInfo = fileIter->storeList[i].second;
        blockStore.set_filename(blockInfo.filename());
        blockStore.set_blockidx(blockInfo.blockidx());
        blockStore.set_blocksize(blockInfo.blocksize());
        blockStore.set_blockhash(blockInfo.blockhash());
        blockStore.set_serveraddress(serverInfo.address());
        blockStore.set_serverhash(serverInfo.hash());
        replyWriter->Write(blockStore);
    }
    return Status::OK;
}

Status NameServerImp::commitGetTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    cout << "$ Client " << context->peer() << " commit transaction: get " << request->filename() << "." << endl;
    return Status::OK;
}

Status NameServerImp::abortGetTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    cout << "$ Client " << context->peer() << " abort transaction: get " << request->filename() << "." << endl;
    return Status::OK;
}

Status NameServerImp::beginPutTransaction(ServerContext *context, const FileInfo *request, ServerWriter<BlockStore> *replyWriter){
    string fileName = request->filename();
    unsigned long fileSize = request->filesize();
    cout << "$ Client " << context->peer() << " begin transaction: put " << fileName << "." << endl;
    auto fileIter = fileList.begin();
    while(fileIter != fileList.end()){
        if(fileIter->fileInfo.filename() == fileName){
            break;
        }
        fileIter++;
    }

    if(fileIter != fileList.end()){
        cout << "# File " << fileName << " already exists!" << endl;
        return Status::CANCELLED;
    }

    unsigned long remainSize = fileSize, blockSize, blockIdx = 0;
    string tempStr = fileName;
    FileMetaData fileMetaData;

    fileMetaData.fileInfo.set_filename(fileName);
    fileMetaData.fileInfo.set_filesize(fileSize);

    default_random_engine engine;
    while(remainSize > 0){
        blockSize = remainSize > maxBlockSize ? maxBlockSize : remainSize;
        tempStr[engine()%tempStr.length()] = (unsigned char)engine();

        // 计算Hash值
        SHA256_CTX stx;
        unsigned char hash[SHA256_DIGEST_LENGTH+1];
        hash[SHA256_DIGEST_LENGTH] = '\0';
        SHA256_Init(&stx);
        SHA256_Update(&stx, tempStr.data(), fileName.length());
        SHA256_Final(hash, &stx);

        auto serverIter = serverList.begin();
        while(serverIter != serverList.end()-1){
            if(memcmp((char*)hash, serverIter->hash().data(), SHA256_DIGEST_LENGTH) <= 0){
                break;
            }
            serverIter++;
        }

        BlockInfo blockInfo;
        blockInfo.set_filename(fileName);
        blockInfo.set_blockidx(blockIdx);
        blockInfo.set_blocksize(blockSize);

        ServerInfo serverInfo;
        serverInfo.set_address(serverIter->address());
        serverInfo.set_hash(serverIter->hash());

        fileMetaData.storeList.push_back(pair<BlockInfo, ServerInfo>(blockInfo, serverInfo));

        blockIdx += 1;
        remainSize -= blockSize;
    }
    tempList.push_back(fileMetaData);
    auto metaIter = fileMetaData.storeList.begin();

    BlockStore blockStore;
    while(metaIter != fileMetaData.storeList.end()){
        BlockInfo blockInfo = metaIter->first;
        ServerInfo serverInfo = metaIter->second;
        blockStore.set_filename(blockInfo.filename());
        blockStore.set_blockidx(blockInfo.blockidx());
        blockStore.set_blocksize(blockInfo.blocksize());
        blockStore.set_blockhash(blockInfo.blockhash());
        blockStore.set_serveraddress(serverInfo.address());
        blockStore.set_serverhash(serverInfo.hash());
        replyWriter->Write(blockStore);
        metaIter++;
    }
    return Status::OK;
}

Status NameServerImp::commitPutTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    cout << "$ Client " << context->peer() << " commit transaction: put " << request->filename() << "." << endl;

    auto fileIter = tempList.begin();
    while(fileIter != tempList.end()){
        if(fileIter->fileInfo.filename() == request->filename()){
            break;
        }
        fileIter++;
    }

    if(fileIter == tempList.end()){
        cout << "# File " << request->filename() << " not found!" << endl;
        return Status::CANCELLED;
    }

    fileList.push_back(*fileIter);
    tempList.erase(fileIter);

    return Status::OK;
}

Status NameServerImp::abortPutTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    cout << "$ Client " << context->peer() << " abort transaction: put " << request->filename() << "." << endl;

    auto fileIter = tempList.begin();
    while(fileIter != tempList.end()){
        if(fileIter->fileInfo.filename() == request->filename()){
            break;
        }
        fileIter++;
    }

    if(fileIter == tempList.end()){
        cout << "# File " << request->filename() << " not found!" << endl;
        return Status::CANCELLED;
    }

    tempList.erase(fileIter);
    return Status::OK;
}

Status NameServerImp::beginRmTransaction(ServerContext *context, const FileInfo *request,ServerWriter<BlockStore> *replyWriter) {
    string fileName = request->filename();
    cout << "$ Client " << context->peer() << " begin transaction: rm " << fileName << "." << endl;
    auto fileIter = fileList.begin();
    while(fileIter != fileList.end()){
        if(fileIter->fileInfo.filename() == fileName){
            break;
        }
        fileIter++;
    }

    if(fileIter == fileList.end()){
        cout << "# File " << fileName << " not found!" << endl;
        return Status::CANCELLED;
    }

    tempList.push_back(*fileIter);

    BlockStore blockStore;
    for(long i = 0; i < fileIter->storeList.size(); i++){
        BlockInfo blockInfo = fileIter->storeList[i].first;
        ServerInfo serverInfo = fileIter->storeList[i].second;
        blockStore.set_filename(blockInfo.filename());
        blockStore.set_blockidx(blockInfo.blockidx());
        blockStore.set_blocksize(blockInfo.blocksize());
        blockStore.set_blockhash(blockInfo.blockhash());
        blockStore.set_serveraddress(serverInfo.address());
        blockStore.set_serverhash(serverInfo.hash());
        replyWriter->Write(blockStore);
    }

    fileList.erase(fileIter);
    return Status::OK;
}

Status NameServerImp::commitRmTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    cout << "$ Client " << context->peer() << " commit transaction: rm " << request->filename() << "." << endl;

    auto fileIter = tempList.begin();
    while(fileIter != tempList.end()){
        if(fileIter->fileInfo.filename() == request->filename()){
            break;
        }
        fileIter++;
    }

    if(fileIter == tempList.end()){
        cout << "# File " << request->filename() << " not found!" << endl;
        return Status::CANCELLED;
    }

    tempList.erase(fileIter);
    return Status::OK;
}

Status NameServerImp::abortRmTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    cout << "$ Client " << context->peer() << " abort transaction: rm " << request->filename() << "." << endl;

    auto fileIter = tempList.begin();
    while(fileIter != tempList.end()){
        if(fileIter->fileInfo.filename() == request->filename()){
            break;
        }
        fileIter++;
    }

    if(fileIter == tempList.end()){
        cout << "# File " << request->filename() << " not found!" << endl;
        return Status::CANCELLED;
    }

    fileList.push_back(*fileIter);
    tempList.erase(fileIter);
    return Status::OK;
}

Status NameServerImp::updateBlockInfo(ServerContext *context, const BlockInfo *request, BlockInfo *reply) {
    string fileName = request->filename();
    unsigned long blockIdx = request->blockidx();

    auto fileIter = tempList.begin();
    while(fileIter != tempList.end()){
        if(fileIter->fileInfo.filename() == fileName){
            break;
        }
        fileIter++;
    }
    if(fileIter == tempList.end()){
        cout << "# File " << fileName << " not exists!" << endl;
        return Status::CANCELLED;
    }

    auto storeIter = fileIter->storeList.begin();
    while(storeIter != fileIter->storeList.end()){
        if(storeIter->first.blockidx() == blockIdx){
            break;
        }
        storeIter++;
    }
    if(storeIter == fileIter->storeList.end()){
        cout << "# Block " << fileName << "_" << blockIdx << " not exists!" << endl;
        return Status::CANCELLED;
    }

    storeIter->first.set_blocksize(request->blocksize());
    storeIter->first.set_blockhash(request->blockhash());

    *reply = *request;
    return Status::OK;
}

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
    NameServerImp service(1024*1024*4);

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "# Name server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    strcpy(SERVER_ADDRESS, argv[1]);
    RunServer();

    return 0;
}