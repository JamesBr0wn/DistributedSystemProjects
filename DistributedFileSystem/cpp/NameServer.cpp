//
// Created by JamesBrown on 2018/12/26.
//

#include "NameServer.h"

int cmp(const NodeInfo& a, const NodeInfo& b){
    return a.hash() < b.hash();
}

Status NameServerImp::StartServer(ServerContext *context, const NodeInfo *request, NodeInfo *reply) {
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

Status NameServerImp::TerminateServer(ServerContext *context, const NodeInfo *request, NodeInfo *reply) {
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