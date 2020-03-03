#pragma once

#include "../catalog/index.h"
#include "../record/value.h"

class indexing
{
public:
    bool createIndex(index idx);
    bool dropIndex(index idx);
    int searchEqual(index idx, value_t &value);
    vector<int> searchRange(index idx, value_t &begin, value_t &end);
    void insertKey(index idx, value_t &v, int blockOffset, int offset);
    void deleteKey(index idx, value_t &v);
};

extern shared_ptr<indexing> indexManager;
