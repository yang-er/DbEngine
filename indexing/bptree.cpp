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
    // ������������С����ֲ���
    int columnLength = index.columnLength;
    // 4k��С�Ŀ�Ҫһ���ֽڷֱ���Ҷ�ӽڵ㻹���м�ڵ㣬�ĸ��ֽڼ�¼��������Ŀ��4���ֽ�˵�����ڵ�Ŀ��
    // ����һ��POINTERLENGTH���ȵ�ָ��ָ����һ���ֵܽڵ�
    // ��ÿ������������8���ֽڵ�ָ�루ǰ�ĸ��ֽڱ�ʾ��¼�ڱ��ļ�����һ�飬���ĸ��ֽڱ�ʾ����tuple��ƫ��������myindexInfo.columnLength���ȵļ�ֵ
    MAX_FOR_LEAF = (int)floor((block_t::SIZE - 1 /*Ҷ�ӱ��*/ - 4 /*��ֵ��*/ - PNT_LEN /*���׿��*/ - PNT_LEN /*��һ��Ҷ�ӿ�Ŀ��*/) / (8.0 + columnLength));
    MIN_FOR_LEAF = (int)ceil(1.0 * MAX_FOR_LEAF / 2);
    MAX_CHILDREN_FOR_INTERNAL = MAX_FOR_LEAF;
    MIN_CHILDREN_FOR_INTERNAL = (int)ceil(1.0 * (MAX_CHILDREN_FOR_INTERNAL) / 2);

    if (index.columnType == 'I') sampleVal.reset(new value_int);
    else if (index.columnType == 'F') sampleVal.reset(new value_float);
    else if (index.columnType == 'S') sampleVal.reset(new value_string);

    if (create)
    {
        // Ϊ��ʼ�ĸ������µĿռ�
        index.rootNumber = 0;
        index.blockCount++;
        // �����������ļ��ĵ�һ�飬����LeafNode���װ��
        std::make_shared<node_leaf>(index.rootNumber, *this);
    }
}

/**
 * ����Ϊ��λ�Ĳ���
 * @param key
 * @param blockOffset
 * @param offset
 */
void bptree::insert(value_t& key, int blockOffset, int offset)
{
    auto rootNode = getNode(index.rootNumber);
    block_id newBlockId = rootNode->insert(key, blockOffset, offset);
    if (newBlockId != -1) //�����з��أ�˵�����鱻������
        index.rootNumber = newBlockId;
}


/**
 * ����Ϊ��λ�ĵ�ֵ����
 * @param key
 * @return
 */
offset_t bptree::search(value_t& key)
{
    auto rootNode = getNode(index.rootNumber);
    return rootNode->search(key);
}


/**
 * ����Ϊ��λ�ķ�Χ����
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
 * ����Ϊ��λ��������ɾ��
 * @param key
 */
void bptree::remove(value_t& key)
{
    auto rootNode = getNode(index.rootNumber);
    block_id newBlockId = rootNode->remove(key);
    if (newBlockId != -1) //�����з��أ�˵�����鱻������
        index.rootNumber = newBlockId;
}
