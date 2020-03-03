#define _CRT_SECURE_NO_WARNINGS
#include "block.h"
#include <cassert>
#include <cstring>

void block_t::read(int byteOffset, void *dest, int length) const
{
    assert(byteOffset + length <= SIZE);
    memcpy(dest, data+byteOffset, length);
}

void block_t::write(int byteOffset, const void *_data, int size)
{
    assert(byteOffset + size <= SIZE);
    memcpy(data+byteOffset, _data, size);
    dirty = true;
}

int block_t::readInt(int byteOffset) const
{
    int retVal;
    read(byteOffset, &retVal, sizeof(int));
    return retVal;
}

float block_t::readFloat(int byteOffset) const
{
    float retVal;
    read(byteOffset, &retVal, sizeof(float));
    return retVal;
}

void block_t::writeInt(int byteOffset, int value)
{
    write(byteOffset, &value, sizeof(int));
}

void block_t::writeFloat(int byteOffset, float value)
{
    write(byteOffset, &value, sizeof(float));
}

string block_t::readString(int byteOffset) const
{
    char to[STR_SIZE];
    memcpy(to, data+byteOffset, STR_SIZE);
    string value(to);
    return value;
}

void block_t::writeString(int byteOffset, const string &value)
{
    assert(value.length() < STR_SIZE);
    assert(byteOffset + STR_SIZE < SIZE);
    int strLen = value.length();
    memcpy(data+byteOffset, value.data(), strLen);
    memset(data+byteOffset+strLen, 0, STR_SIZE-strLen);
    dirty = true;
}

bool block_t::isDirty() const
{
    return dirty;
}

void block_t::setClean()
{
    dirty = false;
}

block_t::block_t()
{
    data = nullptr;
    blockId = -1;
    fileName = "";
    dirty = false;
    pinned = false;
}

void block_t::flushToFile()
{
    FILE* fblock = fopen(fileName.data(), "rb+");
    if (fblock == nullptr)
        fblock = fopen(fileName.data(), "wb+");

    // 先跳转至文件尾部，检查文件长度是否正确。
    fseek(fblock, 0, SEEK_END);
    int len = ftell(fblock);
    if (len < blockId*SIZE)
    {
        int noclen = blockId*SIZE-len;
        char *tow = new char[noclen];
        memset(tow, 0, noclen);
        fwrite(tow, noclen, 1, fblock);
        delete[] tow;
    }

    fseek(fblock, blockId*SIZE, 0);
    fwrite(data, SIZE, 1, fblock);
    fclose(fblock);
    dirty = false;
}

void block_t::slideTo(const string &file, int id, char *mem)
{
    fileName = file;
    blockId = id;
    data = mem;
    dirty = false;
    pinned = false;

    FILE* fblock = fopen(file.data(), "rb+");
    if (fblock == nullptr)
        fblock = fopen(file.data(), "wb+");

    // 先跳转至文件尾部，检查文件长度是否正确。
    fseek(fblock, 0, SEEK_END);
    int len = ftell(fblock);
    if (len < blockId*SIZE)
    {
        int noclen = blockId*SIZE-len;
        char *tow = new char[noclen];
        memset(tow, 0, noclen);
        fwrite(tow, noclen, 1, fblock);
        delete[] tow;
    }

    fseek(fblock, id*SIZE, SEEK_SET);
    fread(data, SIZE, 1, fblock);
    fclose(fblock);
}

char block_t::readChar(int byteOffset) const
{
    assert(byteOffset < SIZE);
    return data[byteOffset];
}

void block_t::writeChar(int byteOffset, char ch)
{
    assert(byteOffset < SIZE);
    data[byteOffset] = ch;
    dirty = true;
}

void block_t::copyTo(int srcPos, block_t& desBlock, int destPos, int length) const
{
    auto tmp = new char[length];
    memcpy(tmp, this->data + srcPos, length);
    memcpy(desBlock.data + destPos, tmp, length);
    delete[] tmp;
    desBlock.dirty = true;
}

void block_t::setPin(bool pin)
{
    pinned = pin;
}
