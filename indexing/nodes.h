#pragma once

#include "../storage/buffer.h"
#include "../record/value.h"
#include "bptree.h"

using block_id = int;
const int SZSL = 16843009;
using offset_t = vector<pair<int,int>>;


class node_t : public std::enable_shared_from_this<node_t>
{
public:
    block_id curBlock;
    bptree& tree;

protected:
    block getBlock(int id = -1) { return bufferManager->getBlock(tree.fileName, id == -1 ? curBlock : id); }

public:
    node_t(block_id blk, bptree& bpt) : curBlock(blk), tree(bpt) {}
    virtual block_id insert(value_t &key, int blockOffset, int offset) = 0;
    virtual block_id remove(value_t &key) = 0;
    virtual offset_t search(value_t &key) = 0;
    virtual offset_t search(value_t &begin, value_t &end) = 0;
};


class node_internal : public node_t
{
public:
    node_internal(block_id blk, bptree& bpt);
    node_internal(block_id blk, bptree& bpt, bool t);

    block_id insert(value_t &key, int blockOffset, int offset) override;
    block_id remove(value_t &key) override;
    offset_t search(value_t &key) override;
    offset_t search(value_t &begin, value_t &end) override;
    block_id branchInsert(value_t &branchKey, const shared_ptr<node_t> &lson, const shared_ptr<node_t> &rson);
    block_id merge(value_t &unionKey, block_id afterBlock);
    shared_ptr<value_t> rearrangeAfter(block_id siblingBlock, value_t &internalKey);
    shared_ptr<value_t> rearrangeBefore(block_id siblingBlock, value_t &internalKey);
    void exchange(value_t &changeKey, int posBlockNum);
    block_id remove(block_id blk);
};


class node_leaf : public node_t
{
public:
    node_leaf(block_id blk, bptree& bpt);
    node_leaf(block_id blk, bptree& bpt, bool t);

    block_id insert(value_t &key, int blockOffset, int offset) override;
    block_id remove(value_t &key) override;
    offset_t search(value_t &key) override;
    offset_t search(value_t &begin, value_t &end) override;
    block_id merge(block_id afterBlock);
    shared_ptr<value_t> rearrangeAfter(block_id siblingBlock);
    shared_ptr<value_t> rearrangeBefore(block_id siblingBlock);
};
