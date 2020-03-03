#include "bptree.h"
#include "nodes.h"
#include <cmath>

std::shared_ptr<node_t> bptree::getNode(int blkId)
{
    block b = bufferManager->getBlock(fileName, blkId);
    if (b.readChar(0) == 'I')
        return std::make_shared<node_internal>(blkId, *this, true);
    else
        return std::make_shared<node_leaf>(blkId, *this, true);
}

bptree::bptree(index_t &index, bool create) : index(index)
{
    fileName = index.indexName + ".index";
    // 根据索引键大小计算分叉数
    int columnLength = index.columnLength;
    // 4k大小的块要一个字节分辨是叶子节点还是中间节点，四个字节记录索引键数目，4个字节说明父节点的块号
    // 还有一个POINTERLENGTH长度的指针指向下一个兄弟节点
    // 而每个索引键包括8个字节的指针（前四个字节表示记录在表文件的那一块，后四个字节表示整个tuple的偏移量）和myindexInfo.columnLength长度的键值
    MAX_FOR_LEAF = (int)floor((block_t::SIZE - 1 /*叶子标记*/ - 4 /*键值数*/ - PNT_LEN /*父亲块号*/ - PNT_LEN /*下一块叶子块的块号*/) / (8.0 + columnLength));
    MIN_FOR_LEAF = (int)ceil(1.0 * MAX_FOR_LEAF / 2);
    MAX_CHILDREN_FOR_INTERNAL = MAX_FOR_LEAF;
    MIN_CHILDREN_FOR_INTERNAL = (int)ceil(1.0 * (MAX_CHILDREN_FOR_INTERNAL) / 2);

    if (index.columnType == 'I') sampleVal.reset(new value_int);
    else if (index.columnType == 'F') sampleVal.reset(new value_float);
    else if (index.columnType == 'S') sampleVal.reset(new value_string);

    if (create)
    {
        // 为初始的根申请新的空间
        index.rootNumber = 0;
        index.blockCount++;
        // 创建该索引文件的第一块，并用LeafNode类包装它
        std::make_shared<node_leaf>(index.rootNumber, *this);
    }
}

/**
 * 以树为单位的插入
 * @param key
 * @param blockOffset
 * @param offset
 */
void bptree::insert(value_t& key, int blockOffset, int offset)
{
    auto rootNode = getNode(index.rootNumber);
    block_id newBlockId = rootNode->insert(key, blockOffset, offset);
    if (newBlockId != -1) //假如有返回，说明根块被更新了
        index.rootNumber = newBlockId;
}


/**
 * 以树为单位的等值查找
 * @param key
 * @return
 */
offset_t bptree::search(value_t& key)
{
    auto rootNode = getNode(index.rootNumber);
    return rootNode->search(key);
}


/**
 * 以树为单位的范围查找
 * @param begin
 * @param end
 * @return
 */
offset_t bptree::search(value_t& begin, value_t& end)
{
    auto rootNode = getNode(index.rootNumber);
    return rootNode->search(begin, end);
}


/**
 * 以树为单位的索引键删除
 * @param key
 */
void bptree::remove(value_t& key)
{
    auto rootNode = getNode(index.rootNumber);
    block_id newBlockId = rootNode->remove(key);
    if (newBlockId != -1) //假如有返回，说明根块被更新了
        index.rootNumber = newBlockId;
}
