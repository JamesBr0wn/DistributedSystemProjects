//
// Created by JamesBrown on 2018/12/26.
//

#include "NameServer.h"

int cmp(const ServerInfo& a, const ServerInfo& b){
    return a.hash() < b.hash();
}

NameServerImp::NameServerImp() {
    BlockInfo blockInfo;
    blockInfo.set_filename("block");
    blockInfo.set_blockidx(0);
    string blockName = blockInfo.filename() + '_' + to_string(blockInfo.blockidx());
    ifstream ifs("Temp/" + blockName, ios::in | ios::binary | ios::ate);
    long blockSize = ifs.tellg();
    cout << "Block " << blockName << " size: " << blockSize << endl;
    blockInfo.set_blocksize(blockSize);
    ifs.seekg(0, ios::beg);
    char * blockData = new char[blockSize];
    ifs.read(blockData, blockSize);

    SHA256_CTX stx;
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    hash[SHA256_DIGEST_LENGTH] = '\0';
    SHA256_Init(&stx);
    SHA256_Update(&stx, blockData, (size_t)blockSize);
    SHA256_Final(hash, &stx);
    blockInfo.set_blockhash((char*)hash);
    delete[] blockData;
    ifs.close();

    FileInfo fileInfo;
    fileInfo.set_filename(blockInfo.filename());
    fileInfo.set_filesize(blockInfo.blocksize());

    FileMetaData meta;
    meta.fileInfo = fileInfo;

    ServerInfo serverInfo;
    serverInfo.set_address("127.0.0.1:8888");
    
    meta.storeList.emplace_back();
}

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
    auto it = fileList.begin();
    while(it != fileList.end()){
        if(it->fileInfo.filename() == fileName){
            break;
        }
        it++;
    }

    if(it == fileList.end()){
        cout << "# File " << fileName << " not found!" << endl;
        return Status::CANCELLED;
    }

    for(long i = 0; i < it->storeList.size(); i++){
        replyWriter->Write(it->storeList[i]);
    }
    return Status::OK;
}

Status NameServerImp::commitGetTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) {
    *reply = *request;
    return Status::OK;
}

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
    NameServerImp service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Name server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    strcpy(SERVER_ADDRESS, argv[1]);
    RunServer();

    return 0;
}