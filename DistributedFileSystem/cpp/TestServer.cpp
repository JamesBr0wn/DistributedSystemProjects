//
// Created by JamesBrown on 2018/12/26.
//
#include <iostream>
#include <string>
#include <fstream>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include "DataServer.grpc.pb.h"

using std::string;
using std::to_string;
using std::ifstream;
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
using grpc::ServerWriter;
using grpc::Status;
using DataServer::DataService;
using DataServer::BlockInfo;
using DataServer::BlockUnit;

char SERVER_ADDRESS[32];

class DataServerImp final: public DataService::Service{
public:
    DataServerImp(string dir, size_t sz):storeDirectory(dir), unitSize(sz) {}

    Status ReadBlock(ServerContext* context, const BlockInfo* request,
                     ServerWriter<BlockUnit>* reply_writer) override {
        string blockName = request->filename() + '_' +
                to_string(request->blockidx());
        ifstream fs(storeDirectory + blockName, ios::in | ios::binary | ios::ate);
        if(fs.is_open()){
            // 打开Block文件，检查blockSize是否相符
            long blockSize = fs.tellg();
            cout << "$ Block " << blockName << " size: " << blockSize << endl;
            if(blockSize != request->blocksize()){
                cout << "# Block " << blockName << " size mismatch!" << endl;
                return Status::CANCELLED;
            }

            // 初始化流写入相关变量
            fs.seekg(0, ios::beg);
            unsigned long unitNum = blockSize / unitSize;
            char* unitData = new char[unitSize];
            BlockUnit unit;

            // 初始化Hash检测相关变量
            SHA256_CTX stx;
            unsigned char hash[SHA256_DIGEST_LENGTH+1];
            hash[SHA256_DIGEST_LENGTH] = '\0';
            SHA256_Init(&stx);

            for(unsigned long i = 0; i < unitNum; i++){
                fs.read(unitData + i * unitSize, unitSize);
                SHA256_Update(&stx, unitData, (size_t)unitSize);
                unit.set_filename(request->filename());
                unit.set_blockidx(request->blockidx());
                unit.set_unitidx(i);
                unit.set_unitdata(unitData, (size_t)unitSize);
                reply_writer->Write(unit);
            }
            if(blockSize % unitSize != 0){
                fs.read(unitData + unitNum * unitSize, blockSize % unitSize);
                SHA256_Update(&stx, unitData, (size_t)(blockSize % unitSize));
                unit.set_filename(request->filename());
                unit.set_blockidx(request->blockidx());
                unit.set_unitidx(unitNum);
                unit.set_unitdata(unitData, (size_t)unitSize);
                reply_writer->Write(unit);
            }

            // 验证Hash值
            SHA256_Final(hash, &stx);
            cout << "$ Block " << blockName << " hash: ";
            cout << hex;
            for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
                cout << (unsigned int)hash[i];
            }
            cout << dec << endl;
            if(memcmp(hash, request->blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
                cout << "# Block " << blockName << " hash mismatch!" << endl;
                return Status::CANCELLED;
            }

            // 返回文件块
            reply->set_filename(request->filename());
            reply->set_blocknum(request->blocknum());
            reply->set_blocksize(request->blocksize());
            reply->set_blockdata(blockData, (size_t)blockSize);
            reply->set_blockhash(request->blockhash());

            delete[] blockData;
            fs.close();

            return Status::OK;

        }else{
            cout << "# Block " << blockName << " open failed!" << endl;
            return Status::CANCELLED;
        }

    }
private:
    string storeDirectory;
    size_t unitSize;
};

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
    DataServerImp service("BlockStore/");

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Info server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    strcpy(SERVER_ADDRESS, argv[1]);
    RunServer();

    return 0;
}
