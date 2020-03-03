#include "nodes.h"
#define MinChildIn tree.MIN_CHILDREN_FOR_INTERNAL
#define MaxChildIn tree.MAX_CHILDREN_FOR_INTERNAL

node_internal::node_internal(block_id blk, bptree &bpt) : node_t(blk, bpt)
{
    block curBlock = getBlock();
    curBlock.writeChar(0, 'I'); // 标识为中间块
    curBlock.writeInt(1, 0); // 现在共有0个key值
    curBlock.writeInt(5, SZSL * '$'); // 说明没有父块标号
}

node_internal::node_internal(block_id blk, bptree &bpt, bool) : node_t(blk, bpt)
{
}


/**
 * 以中间节点为单位的插入
 * @param key 键值
 * @param blockOffset 块号
 * @param offset 块中偏移
 * @return 插入的块号
 */
block_id node_internal::insert(value_t &key, int blockOffset, int offset)
{
    block curBlock = getBlock();
    auto tmp = key.copy();
    int keyNum = curBlock.readInt(1), i = 0; // 获取路标数

    for (; i < keyNum; i++)
    {
        int pos = 9 + PNT_LEN + i * (key.size() + PNT_LEN);
        tmp->read(curBlock, pos);
        if (key.compareTo(*tmp) < 0) break;
    }

    int nextBlockId = curBlock.readInt(9 + i * (key.size() + PNT_LEN));
    auto nextNode = tree.getNode(nextBlockId);
    return nextNode->insert(key, blockOffset, offset);
}


/**
 * 以中间节点为单位的删除
 * @param key 键值
 * @return 删除的块号
 */
block_id node_internal::remove(value_t& key)
{
    block curBlock = getBlock();
    auto tmp = key.copy();
    int keyNum = curBlock.readInt(1), i = 0;

    for (; i < keyNum; i++)
    {
        int pos = 9 + PNT_LEN + i * (key.size() + PNT_LEN);
        tmp->read(curBlock, pos);
        if (key.compareTo(*tmp) < 0) break;
    }

    int nextBlockId = curBlock.readInt(9 + i * (key.size() + PNT_LEN));
    auto nextNode = tree.getNode(nextBlockId);
    return nextNode->remove(key); //递归删除
}


/**
 * 插入过程中有时需要对中间节点进行分支
 * @param branchKey 要新插入的路标
 * @param lson 新路标的左子节点（也就是已经存在的节点）
 * @param rson 新路标的右子节点
 * @return 新根块
 */
block_id node_internal::branchInsert(value_t& branchKey, const shared_ptr<node_t> &lson, const shared_ptr<node_t> &rson)
{
    block curBlock = getBlock();
    int keNum = curBlock.readInt(1); // 获取路标数
    const int INKEYSZ = PNT_LEN + branchKey.size();

    if (keNum == 0) // 全新节点
    {
        keNum++;
        curBlock.writeInt(1, keNum);
        branchKey.write(curBlock, 9 + PNT_LEN);
        curBlock.writeInt(9, lson->curBlock);
        curBlock.writeInt(9 + INKEYSZ, rson->curBlock);
        return this->curBlock; // 将该节点块返回作为新根块
    }
    else if (++keNum > MaxChildIn) //分裂节点
    {
        // 因为不能创建新内存空间，只能用
        // 这种麻烦的方法来分配子块的那些路标和指针
        bool half = false;

        // 创建一个新块并包装
        int newBlockOffset = tree.index.blockCount;
        tree.index.blockCount++;

        block newBlock = getBlock(newBlockOffset);
        auto newNode = std::make_shared<node_internal>(newBlockOffset, tree);

        // 我们知道新插入路标使超过了上限，也就是
        // 现有路标为MAX+1，我们总是为原来的块
        // 保留MIN个路标，这样新开的块有MAX+1-MIN个路标
        curBlock.writeInt(1, MinChildIn);
        newBlock.writeInt(1, MaxChildIn + 1 - MinChildIn);

        // 假如新路标需插在原来的块也就是在MIN之内
        for (int i = 0; i < MinChildIn; i++)
        {
            int pos = 9 + PNT_LEN + i * (branchKey.size() + PNT_LEN);
            shared_ptr<value_t> tmp = branchKey.copy();
            tmp->read(curBlock, pos);

            // 找到了路标插入的位置
            if (branchKey.compareTo(*tmp) >= 0) continue;

            curBlock.copyTo(
                // 把第MIN_CHILDREN_FOR_INTERNA条开始的记录copy到新block
                9 + MinChildIn * INKEYSZ,
                newBlock,
                9,
                PNT_LEN + (MaxChildIn - MinChildIn) * INKEYSZ);

            curBlock.copyTo( // 给新插入条留出位置
                9 + PNT_LEN + i * INKEYSZ,
                curBlock,
                9 + PNT_LEN + (i + 1) * INKEYSZ,
                // 注意还要把原块的最后一个路标保留（不用于自身，但要作为父块的新插入路标）
                (MinChildIn - i) * INKEYSZ - PNT_LEN);

            // 设置新插入信息
            branchKey.write(curBlock, 9 + PNT_LEN + i * INKEYSZ);
            curBlock.writeInt(9 + (i+1) * INKEYSZ, rson->curBlock);
            half = true;
            break;
        }

        // 新路标需插在新开的块也就是它的位置超出了MIN
        if (!half)
        {
            //把第MIN+1条开始的记录copy到新block
            curBlock.copyTo(
                9 + (MinChildIn + 1) * INKEYSZ,
                newBlock,
                9,
                PNT_LEN + (MaxChildIn - MinChildIn - 1) * INKEYSZ);

            auto tmp = branchKey.copy();
            for (int i = 0; i < MaxChildIn - MinChildIn - 1; i++)
            {
                int pos = 9 + PNT_LEN + i * INKEYSZ;
                tmp->read(newBlock, pos);
                if (branchKey.compareTo(*tmp) >= 0) continue;

                // 给新插入条留出位置
                newBlock.copyTo(
                    9 + PNT_LEN + i * INKEYSZ,
                    newBlock,
                    9 + PNT_LEN + (i + 1) * INKEYSZ,
                    (MaxChildIn - MinChildIn - 1 - i) * INKEYSZ
                );

                //设置新插入信息
                branchKey.write(newBlock, 9 + PNT_LEN + i * INKEYSZ);
                newBlock.writeInt(9 + (i+1) * INKEYSZ, rson->curBlock);
                break;
            }
        }

        // 找出原块与新块之间的路标，提供给父节点做分裂用
        shared_ptr<value_t> spiltKey = branchKey.copy();
        spiltKey->read(curBlock, 9 + PNT_LEN + MinChildIn * INKEYSZ);

        newBlock.setPin(true);
        curBlock.setPin(true);

        //更新新块的子块的父亲
        for (int j = 0, keyNumTmp1 = newBlock.readInt(1); j <= keyNumTmp1; j++)
        {
            int childBlockNum = newBlock.readInt(9 + j * INKEYSZ);
            getBlock(childBlockNum).writeInt(5, newBlockOffset);
        }

        getBlock(newBlockOffset);
        getBlock(this->curBlock);
        newBlock.setPin(false);
        curBlock.setPin(false);

        int parentBlockNum;
        if (curBlock.readInt(5) == SZSL * '$') // 没有父节点，则创建父节点
        {
            // 创建新块并包装
            parentBlockNum = tree.index.blockCount;
            block parentBlock = getBlock(parentBlockNum);
            tree.index.blockCount++;

            // 设置父块信息
            curBlock.writeInt(5, parentBlockNum);
            newBlock.writeInt(5, parentBlockNum);
        }
        else
        {
            // 新块的父亲也就是旧块的父亲
            parentBlockNum = curBlock.readInt(5);
            newBlock.writeInt(5, parentBlockNum);
        }

        auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree);
        // 读出父亲块并为其包装，然后再递归分裂
        return parentNode->branchInsert(*spiltKey, shared_from_this(), newNode);
    }
    else // 不需要分裂节点时
    {
        auto tmp = branchKey.copy(); int i;
        for (i = 0; i < keNum - 1; i++)
        {
            tmp->read(curBlock, 9 + PNT_LEN + i * INKEYSZ);
            if (branchKey.compareTo(*tmp) >= 0) continue;

            //找到插入的位置
            curBlock.copyTo(
                9 + PNT_LEN + i * INKEYSZ,
                curBlock,
                9 + PNT_LEN + (i + 1) * INKEYSZ,
                (keNum - 1 - i) * INKEYSZ);

            branchKey.write(curBlock, 9 + PNT_LEN + i * INKEYSZ);
            curBlock.writeInt(9 + (i+1) * INKEYSZ, rson->curBlock);
            curBlock.writeInt(1, keNum);
            return -1;
        }

        if (i == keNum - 1)
        {
            branchKey.write(curBlock, 9 + PNT_LEN + i * INKEYSZ);
            curBlock.writeInt(9 + (i+1) * INKEYSZ, rson->curBlock);
            curBlock.writeInt(1, keNum);
            return -1;
        }
    }

    return -1;
}


/**
 * 删除过程中产生的节点合并，this块和after块以及它们之间的unionKey
 * @param unionKey
 * @param afterBlock
 * @return
 */
block_id node_internal::merge(value_t& unionKey, block_id _afterBlock)
{
    block curBlock = getBlock();
    block afterBlock = getBlock(_afterBlock);
    const int INKEYSZ = PNT_LEN + unionKey.size();
    int keyNum = curBlock.readInt(1);
    int afterkeyNum = afterBlock.readInt(1);

    afterBlock.copyTo(
        9,
        curBlock,
        9 + (keyNum + 1) * INKEYSZ,
        PNT_LEN + afterkeyNum * INKEYSZ
    );

    // 填入unionKey
    unionKey.write(curBlock, 9 + keyNum * INKEYSZ + PNT_LEN);

    // 重新计算路标大小（原路标数+after路标数+unionKey）
    keyNum = keyNum + afterkeyNum + 1;
    curBlock.writeInt(1, keyNum);

    // 找到父块
    int parentBlockNum = curBlock.readInt(5);
    auto parent = std::make_shared<node_internal>(parentBlockNum, tree, true);
    // TODO: 这个块一定是从末尾删除的吗？
    // tree.index.blockCount--;

    // 在父块中删除after块
    return parent->remove(_afterBlock);
}


/**
 * 删除过程中产生的兄弟块内容重排，
 * this块和after块以及它们之间的internalKey，
 * 返回的changeKey是为了更新父块中它们两指针中间的键值
 * @param siblingBlock
 * @param internalKey
 * @return
 */
shared_ptr<value_t> node_internal::rearrangeAfter(block_id _siblingBlock, value_t& internalKey)
{
    block siblingBlock = getBlock(_siblingBlock);
    block curBlock = getBlock();
    int siblingKeyNum = siblingBlock.readInt(1);
    int keyNum = curBlock.readInt(1);
    const int INKEYSZ = PNT_LEN + internalKey.size();

    // 要转移的一条指针内容
    int blockOffset = siblingBlock.readInt(9);
    // 将internalKey和兄弟块的第一条指针复制到this块的尾部，路标数加1
    internalKey.write(curBlock, 9 + PNT_LEN + keyNum * INKEYSZ);
    curBlock.writeInt(9 + (keyNum + 1) * INKEYSZ, blockOffset);
    keyNum++;
    curBlock.writeInt(1, keyNum);

    // 兄弟块的路标数减1，获取兄弟块的第一条键值
    // 作为更新父块的键值，再将兄弟块后面的信息
    // 向前挪一条指针和路标的距离
    siblingKeyNum--;
    siblingBlock.writeInt(1, siblingKeyNum);
    shared_ptr<value_t> changeKey = internalKey.copy();
    changeKey->read(siblingBlock, 9 + PNT_LEN);

    siblingBlock.copyTo(
        9 + INKEYSZ,
        siblingBlock,
        9,
        PNT_LEN + siblingKeyNum * INKEYSZ);
    return changeKey;
}


/**
 * 兄弟节点在其前
 * @param siblingBlock
 * @param internalKey
 * @return
 */
shared_ptr<value_t> node_internal::rearrangeBefore(block_id _siblingBlock, value_t& internalKey)
{
    block siblingBlock = getBlock(_siblingBlock);
    block curBlock = getBlock();
    int siblingKeyNum = siblingBlock.readInt(1);
    int keyNum = curBlock.readInt(1);
    const int INKEYSZ = PNT_LEN + internalKey.size();

    siblingKeyNum--;
    siblingBlock.writeInt(1, siblingKeyNum);

    // 兄弟块的最后一条路标作为父块的更新路标，最后一条指针将转移给this块
    shared_ptr<value_t> changeKey = internalKey.copy();
    changeKey->read(siblingBlock, 9 + PNT_LEN + siblingKeyNum * INKEYSZ);
    int blockOffset = siblingBlock.readInt(9 + (siblingKeyNum + 1) * INKEYSZ);

    // 给新指针和路标让出位置
    curBlock.copyTo(
        9,
        curBlock,
        9 + INKEYSZ,
        PNT_LEN + keyNum * INKEYSZ);

    curBlock.writeInt(9, blockOffset);
    // 插入从兄弟块挪来的这条指针
    internalKey.write(curBlock, 9 + PNT_LEN);
    keyNum++;
    curBlock.writeInt(1, keyNum);
    return changeKey;
}


/**
 *
 * @param changeKey
 * @param posBlockNum
 */
void node_internal::exchange(value_t& changeKey, int posBlockNum)
{
    block curBlock = getBlock();
    const int INKEYSZ = PNT_LEN + changeKey.size();
    int keyNum = curBlock.readInt(1), i = 0;
    for (; i < keyNum; i++)
    {
        int pos = 9 + i * INKEYSZ;
        int blockCount = curBlock.readInt(pos);
        if (blockCount == posBlockNum) break;
    }

    if (i < keyNum)
        changeKey.write(curBlock, 9 + i * INKEYSZ + PNT_LEN);
}


/**
 *
 * @param blk
 * @return
 */
block_id node_internal::remove(block_id blk)
{
    block curBlock = getBlock();
    const int INKEYSZ = PNT_LEN + tree.index.columnLength;
    int keyNum = curBlock.readInt(1);

    for (int i = 0; i <= keyNum; i++)
    {
        int pos = 9 + i * INKEYSZ;
        int ptr = curBlock.readInt(pos);

        if (ptr == blk) // 如果找到了子块标号
        {
            curBlock.copyTo(
                9 + PNT_LEN + (i - 1) * INKEYSZ,
                curBlock,
                9 + PNT_LEN + i * INKEYSZ,
                (keyNum - i) * INKEYSZ);

            keyNum--;
            curBlock.writeInt(1, keyNum);

            if (keyNum >= MinChildIn) return -1;
            //移除后直接结束

            // 没有父节点时
            if (curBlock.readInt(5) == SZSL * '$')
            {
                if (keyNum == 0)
                {
                    //没有路标，只有一个子块标号时，把它的子块作为根块，把this块删除
                    // TODO: tree.index.blockCount--;
                    return curBlock.readInt(9);
                }

                return -1;
            }

            // 找到父亲块
            int parentBlockNum = curBlock.readInt(5);
            block parentBlock = getBlock(parentBlockNum);
            int parentKeyNum = parentBlock.readInt(1);

            // 查找后续兄弟块
            int sibling, j = 0;
            for (; j < parentKeyNum; j++)
            {
                int ppos = 9 + j * INKEYSZ;
                if (this->curBlock == parentBlock.readInt(ppos))
                {
                    // 读到后续兄弟块
                    sibling = parentBlock.readInt(ppos + INKEYSZ);
                    block siblingBlock = getBlock(sibling);
                    auto unionKey = tree.sampleVal->copy();
                    unionKey->read(parentBlock, ppos + PNT_LEN);

                    // 能够合并
                    if (siblingBlock.readInt(1) + keyNum <= MaxChildIn)
                        return merge(*unionKey, sibling);

                    // 两个节点原来都是MIN_FOR_LEAF，delete一个以后的情况没考虑，这个时候会涉及三个节点
                    // 这种情况没有处理，要处理的话将涉及到4个兄弟块，因为this块有MIN-1个路标，
                    // 兄弟块有MIN个路标，向兄弟块挪一个路标显然是白费力气，但是它们却也不一定能够合并，
                    // 所以很麻烦
                    if (siblingBlock.readInt(1) == MinChildIn) return -1;

                    // 不能合并，通过从兄弟块转移一个路标来满足B+树路标最小限制
                    auto parent = std::make_shared<node_internal>(parentBlockNum, tree, true);
                    parent->exchange(*rearrangeAfter(sibling, *unionKey), this->curBlock);
                    return -1;
                }
            }

            //找不后续块，只能找前续兄弟块
            sibling = parentBlock.readInt(9 + (parentKeyNum - 1) * INKEYSZ);
            block siblingBlock = getBlock(sibling);
            auto unionKey = tree.sampleVal->copy();
            unionKey->read(parentBlock, 9 + (parentKeyNum - 1) * INKEYSZ + PNT_LEN);

            // 能够合并
            if (siblingBlock.readInt(1) + keyNum <= MaxChildIn)
            {
                auto s = std::make_shared<node_internal>(sibling, tree, true);
                return s->merge(*unionKey, this->curBlock);
            }

            //没有考虑的情况
            if (siblingBlock.readInt(1) == MinChildIn) return -1;

            //重排的情况
            auto p = std::make_shared<node_internal>(parentBlockNum, tree, true);
            p->exchange(*rearrangeBefore(sibling, *unionKey), sibling);
            return -1;
        }
    }

    return -1;
}


/**
 * 以中间节点为单位的查找
 */
offset_t node_internal::search(value_t& key)
{
    const int INKEYSZ = PNT_LEN + key.size();
    block curBlock = getBlock();
    int keNum = curBlock.readInt(1), i = 0;
    auto tmp = key.copy();

    for (; i < keNum; i++)
    {
        int pos = 9 + PNT_LEN + i * INKEYSZ;
        tmp->read(curBlock, pos);
        if (key.compareTo(*tmp) < 0) break;
    }

    int nextBlockNum = curBlock.readInt(9 + i * INKEYSZ);
    auto nextNode = tree.getNode(nextBlockNum);
    return nextNode->search(key); // 递归查找
}


/**
 *
 * @param begin
 * @param end
 * @return
 */
offset_t node_internal::search(value_t& begin, value_t& end)
{
    const int INKEYSZ = PNT_LEN + begin.size();
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1), i = 0;
    auto tmp = begin.copy();

    for (; i < keyNum; i++)
    {
        int pos = 9 + PNT_LEN + i * INKEYSZ;
        tmp->read(curBlock, pos);
        if (begin.compareTo(*tmp) < 0) break;
    }

    int nextBlockNum = curBlock.readInt(9 + i * INKEYSZ);
    auto nextNode = tree.getNode(nextBlockNum);
    return nextNode->search(begin, end); // 递归查找
}
