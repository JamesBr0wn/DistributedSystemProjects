//
// Created by JamesBrown on 2018/12/26.
//

#ifndef DISTRIBUTEDSYSTEMPROJECTS_DATASERVER_H
#define DISTRIBUTEDSYSTEMPROJECTS_DATASERVER_H

#include <iostream>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <openssl/sha.h>
#include "DistributedFileSystem.grpc.pb.h"

using std::string;
using std::to_string;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using DistributedFileSystem::DataService;
using DistributedFileSystem::BlockInfo;
using DistributedFileSystem::BlockUnit;
using DistributedFileSystem::NameService;
using DistributedFileSystem::ServerInfo;

char DATA_ADDRESS[32];
char NAME_ADDRESS[32];
char STORE_DIR[64];

class DataServerImp final: public DataService::Service {
public:
    DataServerImp(std::shared_ptr<Channel> channel, string dir, size_t sz);
    ~DataServerImp() override;
    Status getBlock(ServerContext* context, const BlockInfo* request, ServerWriter<BlockUnit>* replyWriter) override;
    Status putBlock(ServerContext* context, ServerReader<BlockUnit>* requestReader, BlockInfo* reply) override;
    Status rmBlock(ServerContext* context, const BlockInfo* request, BlockInfo* reply) override;
private:
    std::unique_ptr<NameService::Stub> _stub;
    string storeDirectory;
    size_t unitSize;
};

#endif //DISTRIBUTEDSYSTEMPROJECTS_DATASERVER_H
