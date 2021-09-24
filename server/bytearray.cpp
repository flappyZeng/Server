#include"bytearray.h"
#include"log.h"
#include"private_endian.h"

#include<string>
#include<string.h>
#include<sstream>
#include<fstream>
#include<iomanip>

namespace server{

    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

    ByteArray::Node::Node(size_t s)
        :ptr(new char[s])
        ,size(s)
        ,next(nullptr){
    }

    ByteArray::Node::Node()
        :ptr(nullptr)
        ,size(0)
        ,next(nullptr){
    }

    ByteArray::Node::~Node(){
        if(ptr){
            next = nullptr;
            delete[] ptr;
        }
    }

    //base_size ：链表的默认长度
    ByteArray::ByteArray(size_t base_size)
        :m_baseSize(base_size)
        ,m_position(0)
        ,m_capacity(base_size)
        ,m_size(0)
        ,m_endian(SERVER_BIG_ENDIAN)
        ,m_root(new Node(base_size)){
            m_cur = m_root;
    }

    ByteArray::~ByteArray(){
        Node* tmp = m_root;
        while(tmp){
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }
    
    //字节序控制
    bool ByteArray::isLittleEndian() const{
        return m_endian == SERVER_LITTLE_ENDIAN;
    }

    void ByteArray::SetLittleEndian(bool val){
        if(val){
            m_endian = SERVER_LITTLE_ENDIAN;
        }else{
            m_endian = SERVER_BIG_ENDIAN;
        }
    }

    //write,数字定长
    void ByteArray::writeFint8(int8_t value){
        write(&value, sizeof(value));
    } 
    void ByteArray::writeFuint8(uint8_t value){
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint16(int16_t value){
        //判断字节序是否和机器一致。
        if(m_endian != SERVER_BYTE_ORDER){
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint16(uint16_t value){
        if(m_endian != SERVER_BYTE_ORDER){
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint32(int32_t value){
        if(m_endian != SERVER_BYTE_ORDER){
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint32(uint32_t value){
        if(m_endian != SERVER_BYTE_ORDER){
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint64(int64_t value){
        if(m_endian != SERVER_BYTE_ORDER){
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint64(uint64_t value){
        if(m_endian != SERVER_BYTE_ORDER){
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    //将int32_t转化为uint_32
    static uint32_t EncodeZigzag32(const int32_t& v){
        if(v < 0){
            return ((uint32_t)(-v)) * 2 - 1;  // -1 --> 1  -2 --> 3
        }else{  
            return v * 2;                     // 1 --> 2    2 --> 4
        }
    }

    static uint64_t EncodeZigzag64(const int64_t& v){
        if(v < 0){
            return ((uint64_t)(-v)) * 2 - 1;  // -1 --> 1  -2 --> 3
        }else{  
            return v * 2;                     // 1 --> 2    2 --> 4
        }
    }

    //write,数字变长(压缩)
    void ByteArray::writeInt32(int32_t value){
        writeUint32(EncodeZigzag32(value));
    }
    void ByteArray::writeUint32(uint32_t value){
        uint8_t tmp[5];
        uint8_t i = 0;
        while(value >= 0x80){
            tmp[i++] = (value & 0x7f) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeInt64(int64_t value){
        writeUint64(EncodeZigzag64(value));
    }
    void ByteArray::writeUint64(uint64_t value){
        uint8_t tmp[10];
        uint8_t i = 0;
        while(value >= 0x80){
            tmp[i++] = (value & 0x7f) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }


    void ByteArray::writeFloat(float value){
        uint32_t v;
        memcpy(&v, &value, sizeof(value));
        writeUint32(v);
    }
    void ByteArray::writeDouble(double value){
        uint64_t v;
        memcpy(&v, &value, sizeof(value));
        writeUint64(v);
    }


    //定长输入str， 长度分别是16，32， 64byte.
    void ByteArray::writeStringF16(const std::string& value){
        //长度+str
        writeFuint16(value.size());
        write(value.c_str(), value.size());
    }
    void ByteArray::writeStringF32(const std::string& value){
        writeFuint32(value.size());
        write(value.c_str(), value.size());
    }
    void ByteArray::writeStringF64(const std::string& value){
        writeFuint64(value.size());
        write(value.c_str(), value.size());
    }

    //可变长度压缩int
    void ByteArray::writeStringVint(const std::string& value){
        writeUint64(value.size());
        write(value.c_str(), value.size());
    }
    //未定义长度str
    void ByteArray::writeStringWithoutLength(const std::string& value){
        write(value.c_str(), value.size());
    }

    //read
    

    int8_t   ByteArray::readFint8(){
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }
    uint8_t  ByteArray::readFuint8(){
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }
    #define XX(type) \
        type v; \
        read(&v, sizeof(v));  \
        if(m_endian != SERVER_BYTE_ORDER){ \
            v = byteswap(v); \
        } \
        return v;


    int16_t  ByteArray::readFint16(){
        XX(int16_t);
    }
    uint16_t ByteArray::readFuint16(){
        XX(uint16_t);
    }
    int32_t  ByteArray::readFint32(){
        XX(int32_t);
    }
    uint32_t ByteArray::readFuint32(){
        XX(uint32_t); 
    }
    int64_t  ByteArray::readFint64(){
        XX(int64_t); 
    }
    uint64_t ByteArray::readFuint64(){
        XX(uint64_t);
    }

    //将uint32_t转化为int_32
    static int32_t DecodeZigzag32(const uint32_t& v){
        return (v >> 1) ^ -(v & 1);
    }

    static int64_t DecodeZigzag64(const uint64_t& v){
       return (v >> 1) ^ -(v & 1);
    }

    int32_t  ByteArray::readInt32(){
        return DecodeZigzag32(readUint32());
    }
    uint32_t ByteArray::readUint32(){
        uint32_t result = 0;
        for(int i = 0; i < 32; i += 7){
            uint8_t b = readFuint8();
            if(b < 0x80){
                result |= ((uint32_t)b) << i;
                break;
            }else{
                result |= (((uint32_t)(b & 0x7f)) << i);
            }
        }
        return result;
    }

    int64_t  ByteArray::readInt64(){
        return DecodeZigzag64(readUint64());
    }
    uint64_t ByteArray::readUint64(){
        uint64_t result = 0;
        for(int i = 0; i < 64; i += 7){
            uint8_t b = readFuint8();
            if(b < 0x80){
                result |= ((uint64_t)b) << i;
                break;
            }else{
                result |= (((uint64_t)(b & 0x7f)) << i);
            }
        }
        return result;
    }

    float    ByteArray::readFloat(){
        uint32_t value = readUint32();
        float result;
        memcpy(&result, &value, sizeof(value));
        return result;
    }
    double   ByteArray::readDouble(){
        uint64_t value = readUint32();
        double result;
        memcpy(&result, &value, sizeof(value));
        return result;
    }

    //读取定长string
    std::string ByteArray::readStringF16(){
        uint16_t len = readFuint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0],len);
        return buff;
    }
    std::string ByteArray::readStringF32(){
        uint32_t len = readFuint32();
        std::string buff;
        buff.resize(len);
        read(&buff[0],len);
        return buff;
    }
    std::string ByteArray::readStringF64(){
        uint64_t len = readFuint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0],len);
        return buff;
    }
    
    std::string ByteArray::readStringVint(){
        uint64_t len = readFuint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0],len);
        return buff;
    }

    //清理
    void ByteArray::clear(){
        m_position = m_size = 0;
        m_capacity = m_baseSize;
        Node* temp = m_root->next;
        //只留下头节点
        while(temp){
            m_cur = temp;
            temp = temp -> next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root -> next = nullptr;
    }

    //读写操作
    void ByteArray::write(const void* buf,  size_t size){
        if(size == 0){
            return;
        }
        addCapacity(size);
        
        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        size_t bpos = 0;

        while(size > 0){
            if(ncap >= size){
                memcpy((char*)m_cur->ptr + npos, (const char*)buf + bpos, size);
                if(m_cur->size == (npos + size)){
                    m_cur = m_cur -> next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }else{
                memcpy((char*)m_cur->ptr + npos, (const char*)buf + bpos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }
        }
        if(m_position > m_size){
            m_size = m_position;
        }

    }
    void ByteArray::read(void* buf, size_t size){
        if(size > getReadSize()){
            throw std::out_of_range("not enough len");
        }
        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur -> size - npos;
        size_t bpos = 0;

        while(size > 0){
            if(ncap >= size){
                memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
                if(m_cur->size == (npos + size)){
                    m_cur = m_cur -> next;
                }

                m_position += size;
                bpos += size;
                size = 0;
            }else{
                memcpy((char*)buf + bpos, m_cur -> ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }
        }

    }
    void ByteArray::read(void* buf, size_t size, size_t position) const{
        if(size > getReadSize()){
            throw std::out_of_range("not enough len");
        }
        size_t npos = position % m_baseSize;
        size_t ncap = m_cur -> size - npos;
        size_t bpos = 0;
        Node* cur = m_cur;
        while(size > 0){
            if(ncap >= size){
                memcpy((char*)buf + bpos, cur->ptr + npos, size);
                if(cur->size == (npos + size)){
                    cur = cur -> next;
                }

                position += size;
                bpos += size;
                size = 0;
            }else{
                memcpy((char*)buf + bpos, cur -> ptr + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
        }
    }


    void ByteArray::setPosition(size_t pos){
        if(pos > m_capacity){  //不能大于所有数据的大小
            throw std::out_of_range("set_position out of range");
        }
        m_position = pos;
        //防止越界
        if(m_position > m_size){
            m_size = m_position;
        }
        m_cur = m_root;
        while(pos > m_cur -> size){
            pos -= m_cur -> size;
            m_cur = m_cur -> next;
        }
        if(pos == m_cur->size){
            m_cur = m_cur -> next;
        }
    }

    //写入/读取文件
    bool ByteArray::writeToFile(const std::string& name) const{
        std::ofstream ofs;
        ofs.open(name, std::ios::trunc | std::ios::binary);
        if(!ofs){
            SERVER_LOG_ERROR(g_logger) << "writeToFile name = " << name 
                        << " error, errno = " << errno << " errstr = " << strerror(errno);
            return false;
        }
        int64_t read_size = getReadSize();
        int64_t pos = m_position;
        Node* cur = m_cur;

        while(read_size > 0){
            int diff = pos % m_baseSize;
            int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
            ofs.write(cur->ptr + diff, len);
            cur = cur -> next;
            pos += len;
            read_size -= len;
        }
        ofs.close();
        return true;

    }   
    bool ByteArray::readFromFile(const std::string& name){
        std::ifstream ifs;
        ifs.open(name, std::ios::binary);
        if(!ifs){
            SERVER_LOG_ERROR(g_logger) << "readFromFile name = " << name 
                        << " error, errno = " << errno << " errstr = " << strerror(errno);
            return false;
        }
        std::shared_ptr<char>buff(new char[m_baseSize], [](char* ptr){delete[] ptr;});
        while(!ifs.eof()){
            ifs.read(buff.get(), m_baseSize);
            write(buff.get(), ifs.gcount());
        }
        ifs.close();
        return true;
        
    }

    //控制字节序列的容量
    void ByteArray::addCapacity(size_t size){
        if(size == 0){
            return;
        }
        int old_cap = getCapacity();
        if(old_cap >= size){
            return;
        }
        size = size - old_cap;
        size_t count = (size / m_baseSize) + ((size % m_baseSize) ? 1 : 0);

        Node* temp = m_root;
        while(temp -> next){
            temp = temp -> next;
        }

        Node* first = nullptr;
        for(size_t i = 0; i < count; ++i){
            temp->next = new Node(m_baseSize);
            if(first == nullptr){
                first = temp -> next;
            }
            temp = temp -> next;
            m_capacity += m_baseSize;
        }

        //如果以前的容量为0，重置cur;
        if(old_cap == 0){
            m_cur = first;
        }

    }


    std::string ByteArray::toString() const{
        std::string str;
        str.resize(getReadSize());
        if(str.empty()){
            return str;
        }
        read(&str[0], str.size(), m_position);
        return str;
    }


    std::string ByteArray::toHexString() const{
        std::string str = toString();
        std::stringstream ss;

        for(size_t i = 0; i < str.size(); ++i){
            if(i > 0 && i % 32 == 0){
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex 
                << (int)(uint8_t)str[i] << " ";
        }
        return ss.str();
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len ) const{
        len = len > getReadSize() ? getReadSize() : len;
        if(len == 0){
            return 0;
        } 
        uint64_t size = len;
        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur -> size - npos;
        iovec iov;
        Node* cur = m_cur;

        while(len > 0){
            if(ncap >= len){
                iov.iov_base = cur -> ptr + npos;
                iov.iov_len = len;
                len = 0;
            }else{
                iov.iov_base = cur -> ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur -> next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const{
        len = len > getReadSize() ? getReadSize() : len;
        if(len == 0){
            return 0;
        } 
        uint64_t size = len;
        size_t npos = position % m_baseSize;
        size_t count = position / m_baseSize;
        Node* cur = m_root;
        while(count > 0){
            cur = cur -> next;
            -- count;
        }

        size_t ncap = cur -> size - npos;
        iovec iov;
        

        while(len > 0){
            if(ncap >= len){
                iov.iov_base = cur -> ptr + npos;
                iov.iov_len = len;
                len = 0;
            }else{
                iov.iov_base = cur -> ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur -> next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len ){
        if(len == 0){
            return 0;
        }
        addCapacity(len);
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur -> size - npos;
        struct iovec iov;
        Node* cur = m_cur;
        while(len > 0){
            if(ncap >= len){
                iov.iov_base = cur -> ptr + npos;
                iov.iov_len = len;
                len = 0; 
            }else{
                iov.iov_base = cur -> ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur -> next;
                ncap = cur -> size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

}