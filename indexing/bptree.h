#pragma once

#include "../catalog/index.h"
#include "../record/value.h"
using offset_t = vector<std::pair<int,int>>;

const int PNT_LEN = 4;

class node_t;

class bptree
{
private:
    string fileName;
    index index;
    shared_ptr<value_t> sampleVal;
    friend class node_t;
    friend class node_internal;
    friend class node_leaf;

public:
    int MIN_CHILDREN_FOR_INTERNAL; //�м�ڵ����С·�����
    int MAX_CHILDREN_FOR_INTERNAL; //�м�ڵ�����·�����
    int MIN_FOR_LEAF; //Ҷ�ӽڵ����С������
    int MAX_FOR_LEAF; //Ҷ�ӽڵ�����������

    bptree(index_t &index, bool create);
    void insert(value_t &key, int blockOffset, int offset);
    offset_t search(value_t &key);
    offset_t search(value_t &begin, value_t &end);
    void remove(value_t &key);
    std::shared_ptr<node_t> getNode(int blkId);
};
