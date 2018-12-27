//
// Created by JamesBrown on 2018/12/26.
//

#ifndef DISTRIBUTEDSYSTEMPROJECTS_CLIENT_H
#define DISTRIBUTEDSYSTEMPROJECTS_CLIENT_H

#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include <DataServer.pb.h>
#include <NameServer.pb.h>
#include <NameServer.grpc.pb.h>
#include "DataServer.grpc.pb.h"

using std::string;
using std::vector;
using std::to_string;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::cin;
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;
using grpc::Status;
using DataServer::DataService;
using DataServer::BlockInfo;
using DataServer::BlockUnit;
using NameServer::NameService;
using NameServer::FileInfo;
using NameServer::ServerInfo;
using NameServer::BlockStore;

char SERVER_ADDRESS[16];

class ClientImp{
public:
    ClientImp(std::shared_ptr<Channel> channel, string dir, size_t sz)
        : nameStub(NameService::NewStub(channel)), cacheDirectory(dir), unitSize(sz) {}
    Status getBlock(BlockInfo request);
    Status putBlock(BlockInfo request);
    Status rmBlock(BlockInfo request);
    Status get(string fileName);
private:
    std::unique_ptr<NameService::Stub> nameStub;
    std::unique_ptr<DataService::Stub> dataStub;
    string cacheDirectory;
    size_t unitSize;
};

#endif //DISTRIBUTEDSYSTEMPROJECTS_CLIENT_H
