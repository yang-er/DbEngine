#include "indexing.h"
#include "bptree.h"
#include "../catalog/catalog.h"
#include "../storage/buffer.h"

/**
 * 创建索引
 * @param idx
 * @return
 */
bool indexing::createIndex(index idx)
{
    bptree tree(idx, true);
    table t = catalogManager->getTable(idx.tableName);

    shared_ptr<value_t> val;
    field f = t.getField(idx.columnId);
    if (f.type == 'I') val.reset(new value_int);
    else if (f.type == 'F') val.reset(new value_float);
    else if (f.type == 'S') val.reset(new value_string);

    int tplen = t.getTupleLength();
    int tpblock = block_t::SIZE / (4 + tplen);
    for (int tupleOffset = 1, count = 1; count <= t.getTupleCount(); tupleOffset++, count++)
    {
        int blockOffset = tupleOffset / tpblock;
        block blk = bufferManager->getBlock(idx.tableName + ".table", blockOffset);
        int byteOffset = (4 + tplen) * (tupleOffset % tpblock);
        if (blk.readInt(byteOffset) >= 0) continue;
        byteOffset += 4;
        val->read(blk, byteOffset + t.getField(idx.columnId).offset);
        blk.setPin(true);
        tree.insert(*val, blockOffset, tupleOffset);
        blk.setPin(false);
    }

    return true;
}


/**
 * 删除索引
 * @param idx
 * @return
 */
bool indexing::dropIndex(index_t &idx)
{
    bufferManager->setInvalid(idx.indexName + ".index");
    return true;
}


/**
 * 等值查找
 * @param idx
 * @param value
 * @return
 */
int indexing::searchEqual(index idx, value_t& value)
{
    bptree tree(idx, false);
    auto result = tree.search(value);
    if (result.empty()) return -1;
    return result.front().second;
}


/**
 * 根据范围搜索值
 * @param idx
 * @param begin
 * @param end
 * @return
 */
vector<int> indexing::searchRange(index idx, value_t& begin, value_t& end)
{
    bptree tree(idx, false);
    auto result = tree.search(begin, end);
    vector<int> retVal(result.size());
    for (int i = 0; i < result.size(); i++)
        retVal[i] = result[i].second;
    return retVal;
}


/**
 * 插入新索引值，已有索引则更新位置信息
 * @param idx
 * @param v
 * @param blockOffset
 * @param offset
 */
void indexing::insertKey(index idx, value_t& v, int blockOffset, int offset)
{
    bptree tree(idx, false);
    tree.insert(v, blockOffset, offset);
}


/**
 * 删除索引值，没有该索引则什么也不做
 * @param idx
 * @param v
 */
void indexing::deleteKey(index idx, value_t& v)
{
    bptree tree(idx, false);
    tree.remove(v);
}


shared_ptr<indexing> indexManager;
