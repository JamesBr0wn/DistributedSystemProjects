//
// Created by JamesBrown on 2018/12/26.
//

#ifndef DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H
#define DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include "NameServer.grpc.pb.h"

using std::string;
using std::to_string;
using std::cout;
using std::endl;
using std::vector;
using std::sort;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using NameServer::NameService;
using NameServer::NodeInfo;

char SERVER_ADDRESS[32];

class NameServerImp final: public NameService::Service {
public:
    NameServerImp() {}
    Status StartServer(ServerContext* context, const NodeInfo* request, NodeInfo* reply) override;
    Status TerminateServer(ServerContext* context, const NodeInfo* request, NodeInfo* reply) override;
private:
    vector<NodeInfo> serverList;
};


#endif //DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H
