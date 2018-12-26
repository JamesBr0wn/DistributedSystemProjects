//
// Created by JamesBrown on 2018/12/26.
//
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include <DataServer.pb.h>
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
using grpc::ClientReader;
using grpc::ClientWriter;
using grpc::Status;
using DataServer::DataService;
using DataServer::BlockInfo;
using DataServer::BlockUnit;

char SERVER_ADDRESS[16];

class TestClient{
public:
    TestClient(std::shared_ptr<Channel> channel, string dir, size_t sz)
        : _stub(DataService::NewStub(channel)), cacheDirectory(dir), unitSize(sz) {}

    Status ReadBlock(const BlockInfo request){
        BlockUnit unit;
        ClientContext context;
        std::unique_ptr<ClientReader<BlockUnit>> reader(
                _stub->ReadBlock(&context, request));
        Status status;

        string blockName = request.filename() + '_' + to_string(request.blockidx());
        ofstream ofs(cacheDirectory + blockName, ios::out | ios::binary | ios::ate);
        if(!ofs.is_open()){
            cout << "# Block " << blockName << " open failed!" << endl << endl;
            return Status::CANCELLED;
        }

        unsigned long i = 0;
        while (reader->Read(&unit)) {
            ofs.write(unit.unitdata().data(), unit.unitdata().length());
            cout << "$ Block " << blockName << " read unit " << i << endl;
            i++;
        }
        ofs.close();
        status = reader->Finish();

        if (status.ok()) {
            cout << "$ Block " << blockName << " read finished!" << endl << endl;
            return Status::OK;
        } else {
            cout << "# Block " << blockName << " read failed!" << endl << endl;
            return Status::CANCELLED;
        }
    }

    Status WriteBlock(const BlockInfo request){
        BlockInfo reply;
        ClientContext context;
        Status status;
        std::unique_ptr<ClientWriter<BlockUnit>> writer(_stub->WriteBlock(&context, &reply));

        string fileName = request.filename();
        unsigned long blockIdx = request.blockidx();
        long blockSize;
        string blockName = fileName + '_' + to_string(blockIdx);
        ifstream ifs(cacheDirectory + blockName, ios::in | ios::binary | ios::ate);
        if(!ifs.is_open()){
            cout << "# Block " << blockName << " open failed!" << endl << endl;
            return Status::CANCELLED;
        }
        blockSize = ifs.tellg();
        cout << "$ Block " << blockName << " size: " << blockSize << endl;

        // 初始化流传输相关变量
        ifs.seekg(0, ios::beg);
        unsigned long unitNum = blockSize / unitSize;
        char* unitData = new char[unitSize];
        BlockUnit unit;

        // 初始化Hash检测相关变量
        SHA256_CTX stx;
        unsigned char hash[SHA256_DIGEST_LENGTH+1];
        hash[SHA256_DIGEST_LENGTH] = '\0';
        SHA256_Init(&stx);

        // 读取Block并传输
        for(unsigned long i = 0; i < unitNum; i++){
            ifs.read(unitData, unitSize);
            SHA256_Update(&stx, unitData, (size_t)unitSize);
            unit.set_filename(fileName);
            unit.set_blockidx(blockIdx);
            unit.set_unitidx(i);
            unit.set_unitdata(unitData, (size_t)unitSize);
            unit.set_lastunit(blockSize % unitSize == 0 && i == unitNum - 1);
            cout << "$ Block " << blockName << " write unit " << i << endl;
            writer->Write(unit);
        }
        if(blockSize % unitSize != 0){
            ifs.read(unitData, blockSize % unitSize);
            SHA256_Update(&stx, unitData, (size_t)(blockSize % unitSize));
            unit.set_filename(fileName);
            unit.set_blockidx(blockIdx);
            unit.set_unitidx(unitNum);
            unit.set_unitdata(unitData, (size_t)blockSize % unitSize);
            unit.set_lastunit(true);
            cout << "$ Block " << blockName << " write unit " << unitNum << endl;
            writer->Write(unit);
        }

        delete[] unitData;
        ifs.close();
        writer->WritesDone();
        status = writer->Finish();

        if (status.ok()) {
            // 检查blockSize是否相符
            if(blockSize != reply.blocksize()){
                cout << "# Block " << blockName << " size mismatch!" << endl << endl;
                return Status::CANCELLED;
            }

            // 验证Hash值
            SHA256_Final(hash, &stx);
            cout << "$ Block " << blockName << " hash: ";
            cout << hex;
            for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
                cout << (unsigned int)hash[i];
            }
            cout << dec << endl;
            if(memcmp(hash, reply.blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
                cout << "# Block " << blockName << " hash mismatch!" << endl << endl;
                return Status::CANCELLED;
            }

            cout << "$ Block " << blockName << " write finished!" << endl << endl;
            return Status::CANCELLED;
        } else {
            cout << "# Block " << blockName << " write failed!" << endl << endl;
            return Status::CANCELLED;
        }
    }

    Status RemoveBlock(const BlockInfo request){
        BlockInfo reply;
        ClientContext context;
        Status status;

        status = _stub->RemoveBlock(&context, request, &reply);
    }

    bool ReadBlockTest(){
        BlockInfo request;
        request.set_filename("block");
        request.set_blockidx(0);
        string blockName = request.filename() + '_' + to_string(request.blockidx());
        ifstream ifs("Temp/" + blockName, ios::in | ios::binary | ios::ate);
        long blockSize = ifs.tellg();
        cout << "Block " << blockName << " size: " << blockSize << endl;
        request.set_blocksize(blockSize);
        ifs.seekg(0, ios::beg);
        char * blockData = new char[blockSize];
        ifs.read(blockData, blockSize);

        SHA256_CTX stx;
        unsigned char hash[SHA256_DIGEST_LENGTH+1];
        hash[SHA256_DIGEST_LENGTH] = '\0';
        SHA256_Init(&stx);
        SHA256_Update(&stx, blockData, (size_t)blockSize);
        SHA256_Final(hash, &stx);
        cout << "Block " << blockName << " hash: ";
        cout << hex;
        for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
            cout << (unsigned int)hash[i];
        }
        cout << dec << endl;
        request.set_blockhash((char*)hash);
        delete[] blockData;
        ifs.close();

        ReadBlock(request);

        getchar();
        getchar();

        RemoveBlock(request);

        return true;
    }

    bool WriteBlockTest(){
        BlockInfo request;
        request.set_filename("block");
        request.set_blockidx(0);
        WriteBlock(request);
        return true;
    }
private:
    std::unique_ptr<DataService::Stub> _stub;
    string cacheDirectory;
    size_t unitSize;
};

int main(int argc, char** argv){
    strcpy(SERVER_ADDRESS, argv[1]);
    TestClient client(grpc::CreateChannel(
            std::string(SERVER_ADDRESS), grpc::InsecureChannelCredentials()), "BlockCache/", 1024 * 1024);
    if(client.ReadBlockTest()){
        std::cout << "$ Read succeed!" << std::endl;
    }else{
        std::cout << "# Read failed!" << std::endl;
    }
    getchar();
    if(client.WriteBlockTest()){
        std::cout << "$ Write succeed!" << std::endl;
    }else{
        std::cout << "# Write failed!" << std::endl;
    }

    return 0;
}