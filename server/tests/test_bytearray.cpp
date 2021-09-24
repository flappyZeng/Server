#include"../bytearray.h"
#include"../log.h"
#include"../macro.h"
#include<stdlib.h>

static server::Logger::ptr g_logger = SERVER_LOG_ROOT();
void test(){
#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type>vec; \
    for(int i = 0; i < len; ++i){ \
        vec.push_back(rand() * rand()); \
    } \
    server::ByteArray::ptr ba(new server::ByteArray(base_len)); \
    for(type& val : vec){ \
        ba->write_fun(val); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i){ \
        type v = ba->read_fun(); \
        SERVER_ASSERT(v == vec[i]); \
    } \
    SERVER_ASSERT(ba->getReadSize() == 0); \
    ba->setPosition(0);\
    SERVER_LOG_INFO(g_logger) << "test valid, test type = " << #type << " size = " << ba -> getSize();\
    SERVER_ASSERT( ba->writeToFile("/tmp/" #type "_"  #len "_" #read_fun ".dat")); \
    server::ByteArray::ptr ba2(new server::ByteArray(base_len * 2)); \
    SERVER_ASSERT( ba2->readFromFile("/tmp/" #type "_"  #len "_" #read_fun ".dat")); \
    ba2 -> setPosition(0); \
    SERVER_ASSERT( ba->toString() == ba2->toString()); \
    SERVER_ASSERT( ba-> getPosition() == 0); \
}
    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX

}

int main(int argc, char** argv){
    test();
    return 0;
}