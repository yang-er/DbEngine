#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
using std::string;

class block_t
{
private:
    char* data;
    string fileName;
    int blockId;
    bool dirty;
    bool pinned;
    friend class buffer_t;
    void flushToFile();
    void slideTo(const string &file, int id, char* mem);
    ~block_t() = default;

public:
    static const int SIZE = 4096;
    static const int STR_SIZE = 256;
    block_t();
    void read(int byteOffset, void* dest, int length) const;
    void write(int byteOffset, const void* data, int size);
    char readChar(int byteOffset) const;
    void writeChar(int byteOffset, char ch);
    int readInt(int byteOffset) const;
    void writeInt(int byteOffset, int value);
    float readFloat(int byteOffset) const;
    void writeFloat(int byteOffset, float value);
    string readString(int byteOffset) const;
    void writeString(int byteOffset, const string &value);
    bool isDirty() const;
    void copyTo(int srcPos, block_t& desBlock, int destPos, int length) const;
    void setClean();
    void setPin(bool pin);
};

typedef block_t &block;
