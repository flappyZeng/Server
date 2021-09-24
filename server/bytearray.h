#pragma once 

#include<memory>
#include<string.h>
#include<vector>
#include <sys/uio.h>

namespace server{

//字节序列化
class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    //节点存储数据，使用链表实现
    struct Node{
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;
        Node* next;
        size_t size;
    };
    //base_size ：链表的默认长度
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //write,数字定长
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    //write,数字变长(压缩)
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);


    void writeFloat(float value);
    void writeDouble(double value);


    //定长输入str， 长度分别是16，32， 64byte.
    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);

    //可变长度压缩int
    void writeStringVint(const std::string& value);
    //未定义长度str
    void writeStringWithoutLength(const std::string& value);

    //read

    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float    readFloat();
    double   readDouble();

    //读取定长string
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    
    std::string readStringVint();

    //清理
    void clear();

    //读写操作
    void write(const void* buf,  size_t size);
    void read(void* buf, size_t size);

    //从某个位置开始读定长的数据
    void read(void* buf, size_t size, size_t  position) const; 

    size_t getPosition()  const {return m_position;}
    void setPosition(size_t pos);

    //写入/读取文件
    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);

    //没有改操作
    size_t getBaseSize() const {return m_baseSize;}

    //返回当前可读的数据量
    size_t getReadSize() const {return m_size - m_position;}

    size_t getSize() const {return m_size;}
    
    bool isLittleEndian() const;
    void SetLittleEndian(bool val);

    std::string toString() const;
    std::string toHexString() const;

    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len );


private:
    //控制字节序列的容量
    void addCapacity(size_t size);
    size_t getCapacity() const {return m_capacity - m_position;}


private:
    size_t m_baseSize;   //Node的大小
    size_t m_position;   //当前操作的位置在那
    size_t m_capacity;   //容量
    size_t m_size;       //当前的数据量大小
    int8_t m_endian;     //存储的字节序
    //链表root节点
    Node* m_root;
    //链表当前节点
    Node* m_cur;

};

}