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
#include <random>
#include <grpcpp/grpcpp.h>
#include <openssl/sha.h>
#include "DistributedFileSystem.grpc.pb.h"

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
using std::default_random_engine;
using std::uniform_int_distribution;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using DistributedFileSystem::NameService;
using DistributedFileSystem::ServerInfo;
using DistributedFileSystem::FileInfo;
using DistributedFileSystem::BlockInfo;
using DistributedFileSystem::BlockStore;

struct FileMetaData{
    FileInfo fileInfo;
    vector<pair<BlockInfo, ServerInfo>> storeList;
};

class NameServerImp final: public NameService::Service {
public:
    NameServerImp(unsigned long sz);
    Status startServer(ServerContext* context, const ServerInfo* request, ServerInfo* reply) override;
    Status terminateServer(ServerContext* context, const ServerInfo* request, ServerInfo* reply) override;
    Status beginGetTransaction(ServerContext *context, const FileInfo *request, ServerWriter<BlockStore> *replyWriter) override;
    Status commitGetTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
    Status abortGetTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
    Status beginPutTransaction(ServerContext *context, const FileInfo *request, ServerWriter<BlockStore> *replyWriter) override;
    Status commitPutTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
    Status abortPutTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
    Status beginRmTransaction(ServerContext *context, const FileInfo *request, ServerWriter<BlockStore> *replyWriter) override;
    Status commitRmTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
    Status abortRmTransaction(ServerContext *context, const FileInfo *request, FileInfo *reply) override;
    Status updateBlockInfo(ServerContext *context, const BlockInfo *request, BlockInfo *reply) override;
private:
    vector<ServerInfo> serverList;
    vector<FileMetaData> fileList;
    vector<FileMetaData> tempList;
    unsigned long maxBlockSize;
};


#endif //DISTRIBUTEDSYSTEMPROJECTS_NAMESERVER_H
