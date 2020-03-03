#pragma once

#include "block.h"
#include <unordered_set>
#include <unordered_map>
#include <memory>
using std::string;
using std::pair;
using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;

namespace std
{
    template<> struct hash<pair<string, int>>
    {
        size_t operator()(const pair<string, int>& rhs) const
        {
            return hash<string>()(rhs.first) ^ hash<int>()(rhs.second);
        }
    };
};

class buffer_t
{
private:
    char* memoryArea;
    static const int MAXMEMBLK = 24;
    int L[MAXMEMBLK+2], R[MAXMEMBLK+2];
    block_t blks[MAXMEMBLK+2];
    unordered_map<pair<string,int>,int> mp;

public:
    buffer_t();
    ~buffer_t();
    block getBlock(const string &file, int id);
    void setInvalid(const string &file);
};

extern shared_ptr<buffer_t> bufferManager;
