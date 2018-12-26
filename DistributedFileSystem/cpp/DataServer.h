//
// Created by JamesBrown on 2018/12/26.
//

#ifndef DISTRIBUTEDSYSTEMPROJECTS_DATASERVER_H
#define DISTRIBUTEDSYSTEMPROJECTS_DATASERVER_H

#include <iostream>
#include <string>
#include <fstream>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include "DataServer.grpc.pb.h"

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
using DataServer::DataService;
using DataServer::BlockInfo;
using DataServer::BlockUnit;

char SERVER_ADDRESS[32];

class DataServerImp final: public DataService::Service {
public:
    DataServerImp(string dir, size_t sz):storeDirectory(dir), unitSize(sz) {}
    Status ReadBlock(ServerContext* context, const BlockInfo* request, ServerWriter<BlockUnit>* replyWriter) override;
    Status WriteBlock(ServerContext* context, ServerReader<BlockUnit>* requestReader, BlockInfo* reply) override;
    Status RemoveBlock(ServerContext* context, const BlockInfo* request, BlockInfo* reply) override;
private:
    string storeDirectory;
    size_t unitSize;
};

#endif //DISTRIBUTEDSYSTEMPROJECTS_DATASERVER_H
