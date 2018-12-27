//
// Created by JamesBrown on 2018/12/26.
//

#ifndef DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H
#define DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include "NameServer.grpc.pb.h"

using std::string;
using std::to_string;
using std::cout;
using std::endl;
using std::vector;
using std::pair;
using std::sort;
using std::ios;
using std::ifstream;
using std::ofstream;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using NameServer::NameService;
using NameServer::ServerInfo;
using NameServer::FileInfo;
using NameServer::BlockInfo;
using NameServer::BlockStore;

char SERVER_ADDRESS[32];

struct FileMetaData{
    FileInfo fileInfo;
    vector<BlockStore> storeList;
};

class NameServerImp final: public NameService::Service {
public:
    NameServerImp();
    Status startServer(ServerContext* context, const ServerInfo* request, ServerInfo* reply) override;
    Status terminateServer(ServerContext* context, const ServerInfo* request, ServerInfo* reply) override;
    Status beginGetTransaction(ServerContext *context, const FileInfo *request, ServerWriter<BlockStore> *replyWriter) override;
    Status commitGetTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
private:
    vector<ServerInfo> serverList;
    vector<FileMetaData> fileList;
};


#endif //DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H
