#include "nodes.h"
#define MinLef tree.MIN_FOR_LEAF
#define MaxLef tree.MAX_FOR_LEAF

node_leaf::node_leaf(block_id blk, bptree& bpt) : node_t(blk, bpt)
{
    block curBlock = getBlock();
    curBlock.writeChar(0, 'L'); // 标识为叶子块
    curBlock.writeInt(1, 0); // 现在共有0个key值
    curBlock.writeInt(5, SZSL * '$');
    curBlock.writeInt(9, SZSL * '&');
}

node_leaf::node_leaf(block_id blk, bptree& bpt, bool t) : node_t(blk, bpt)
{
}

void setKeydata(block b, int pos, value_t& key, int blockOffset, int offset)
{
    b.writeInt(pos, blockOffset);
    b.writeInt(pos + 4, offset);
    key.write(b, pos + 8);
}


/**
 * 以叶子节点为单位的索引的插入
 * @param key
 * @param blockOffset
 * @param offset
 * @return
 */
block_id node_leaf::insert(value_t& key, int blockOffset, int offset)
{
    const int CLINT = tree.index.columnLength + 8;
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1);

    if (++keyNum > MaxLef) // 分裂节点
    {
        bool half = false;
        int newBlockOffset = tree.index.blockCount;
        tree.index.blockCount++;
        block newBlock = getBlock(newBlockOffset);
        auto newNode = std::make_shared<node_leaf>(newBlockOffset, tree);
        shared_ptr<value_t> tmp = key.copy();

        for (int i = 0; i < MinLef - 1; i++) // 插入原块
        {
            int pos = 17 + i * CLINT;
            tmp->read(curBlock, pos);
            if (key.compareTo(*tmp) >= 0) continue;

            curBlock.copyTo( // 把第MinLef-1条开始的记录copy到新block
                9 + (MinLef - 1) * CLINT,
                newBlock,
                9,
                PNT_LEN + (MaxLef - MinLef + 1) * CLINT
            );

            curBlock.copyTo( // 给新插入条留出位置
                9 + i * CLINT,
                curBlock,
                9 + (i + 1) * CLINT,
                PNT_LEN + (MinLef - 1 - i) * CLINT
            );

            // 插入新内容
            setKeydata(curBlock, 9 + i * CLINT, key, blockOffset, offset);
            half = true;
            break;
        }

        if (!half) // 插入新块
        {
            curBlock.copyTo(
                9 + (MinLef) * CLINT,
                newBlock,
                9,
                PNT_LEN + (MaxLef - MinLef) * CLINT);

            int i = 0;
            for (; i < MaxLef - MinLef; i++)
            {
                int pos = 17 + i * CLINT;
                tmp->read(newBlock, pos);
                if (key.compareTo(*tmp) < 0) {
                    newBlock.copyTo(
                        9 + i * CLINT,
                        newBlock,
                        9 + (i + 1) * CLINT,
                        PNT_LEN + (MaxLef - MinLef - i) * CLINT);

                    // 插入新内容
                    setKeydata(newBlock, 9 + i * CLINT, key, blockOffset, offset);
                    break;
                }
            }

            if (i == MaxLef - MinLef)
            {
                newBlock.copyTo( // 给新插入条留出位置
                    9 + i * CLINT,
                    newBlock,
                    9 + (i + 1) * CLINT,
                    PNT_LEN + (MaxLef - MinLef - i) * CLINT);

                // 插入新内容
                setKeydata(newBlock, 9 + i * CLINT, key, blockOffset, offset);
            }
        }

        curBlock.writeInt(1, MinLef);
        newBlock.writeInt(1, MaxLef + 1 - MinLef);

        // 原块最后添上新块的块号，以做成所有叶子节点的一串链表
        curBlock.writeInt(9 + MinLef * CLINT, newBlockOffset);

        int parentBlockNum;
        shared_ptr<node_internal> parentNode;

        if (curBlock.readInt(5) == '$' * SZSL) // 没有父节点，则创建父节点
        {
            parentBlockNum = tree.index.blockCount;
            tree.index.blockCount++;
            curBlock.writeInt(5, parentBlockNum);
            newBlock.writeInt(5, parentBlockNum);
            parentNode = std::make_shared<node_internal>(parentBlockNum, tree);
        }
        else
        {
            parentBlockNum = curBlock.readInt(5);
            newBlock.writeInt(5, parentBlockNum); // 新节点的父亲也就是旧节点的父亲
            parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
        }

        // 将branchKey(新块的第一个索引键值)提交，让父块分裂出它们两个块
        shared_ptr<value_t> branchKey = key.copy();
        branchKey->read(newBlock, 17);
        return parentNode->branchInsert(*branchKey, shared_from_this(), newNode);
    }
    else // 不需要分裂节点时
    {
        if (keyNum - 1 == 0)
        {
            curBlock.copyTo(
                9,
                curBlock,
                9 + CLINT,
                PNT_LEN);

            setKeydata(curBlock, 9, key, blockOffset, offset);
            curBlock.writeInt(1, keyNum);
            return -1;
        }

        int i = 0;
        shared_ptr<value_t> tmp = key.copy();
        for (; i < keyNum; i++)
        {
            int pos = 17 + i * CLINT;
            tmp->read(curBlock, pos);

            if (key.compareTo(*tmp) == 0) // 已有键值则更新
            {
                setKeydata(curBlock, 9 + i * CLINT, key, blockOffset, offset);
                return -1;
            }

            tmp->read(curBlock, pos);
            if (key.compareTo(*tmp) < 0) // 找到插入的位置
            {
                curBlock.copyTo(
                    9 + i * CLINT,
                    curBlock,
                    9 + (i + 1) * CLINT,
                    PNT_LEN + (keyNum - 1 - i) * CLINT);

                setKeydata(curBlock, 9 + i * CLINT, key, blockOffset, offset);
                curBlock.writeInt(1, keyNum);
                return -1;
            }
        }

        if (i == keyNum)
        {
            curBlock.copyTo(
                9 + (i - 1) * CLINT,
                curBlock,
                9 + i * CLINT,
                PNT_LEN);

            setKeydata(curBlock, 9 + (i - 1) * CLINT, key, blockOffset, offset);
            curBlock.writeInt(1, keyNum);
            return -1;
        }
    }

    return -1;
}


/**
 * 以叶子节点为单位的索引删除
 */
block_id node_leaf::remove(value_t& key)
{
    const int CLINT = tree.index.columnLength + 8;
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1);

    shared_ptr<value_t> tmp = key.copy();
    for (int i = 0; i < keyNum; i++)
    {
        int pos = 17 + i * CLINT;
        tmp->read(curBlock, pos);
        if (key.compareTo(*tmp) < 0) // 没找到索引键值
            return -1;
        if (key.compareTo(*tmp) > 0)
            continue;

        //找到对应的键值
        curBlock.copyTo( //移除这条索引
            9 + (i + 1) * CLINT,
            curBlock,
            9 + i * CLINT,
            PNT_LEN + (keyNum - 1 - i) * CLINT);
        keyNum--;
        curBlock.writeInt(1, keyNum);

        if (keyNum >= MinLef) return -1; //仍然满足数量要求
        if (curBlock.readInt(5) == '$' * SZSL) return -1;  // 没有父块，本身为根

        bool lastFlag = false;
        if (curBlock.readInt(9 + keyNum * CLINT) == '&' * SZSL) lastFlag = true; // 叶子块链表的最后一块

        int sibling = curBlock.readInt(9 + keyNum * CLINT);
        block_t* siblingBlock = &getBlock(sibling);
        int parentBlockNum = curBlock.readInt(5);

        // 没有找到后续兄弟节点
        /* 虽然有后续块但不是同一个父亲的兄弟块 */
        if (lastFlag || siblingBlock->readInt(5) != parentBlockNum)
        {
            // 通过父块找前续兄弟块
            block parentBlock = getBlock(parentBlockNum);
            int parentKeyNum = parentBlock.readInt(1), j = 0;

            for (; j < parentKeyNum; j++)
            {
                int ppos = 9 + PNT_LEN + j * (4 + key.size());
                tmp->read(parentBlock, pos);
                if (key.compareTo(*tmp) >= 0) continue;
                sibling = parentBlock.readInt(ppos - CLINT);
                siblingBlock = &getBlock(sibling);
                break;
            }

            //可以合并
            if ((siblingBlock->readInt(1) + keyNum) <= MaxLef)
            {
                auto sbNode = std::make_shared<node_leaf>(sibling, tree, true);
                return sbNode->merge(this->curBlock);
            }

            // 两个节点原来都是MIN_FOR_LEAF，delete一个以后的情况没考虑，这个时候会涉及三个节点
            if (siblingBlock->readInt(1) == MinLef) return -1;

            // 重排
            auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
            parentNode->exchange(*rearrangeBefore(sibling), sibling);
            return -1;
        }

        // 合并
        if (siblingBlock->readInt(1) + keyNum <= MaxLef)
            return this->merge(sibling);

        // 没考虑的情况
        if (siblingBlock->readInt(1) == MinLef) return -1;

        // 重排
        auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
        parentNode->exchange(*rearrangeAfter(sibling), this->curBlock);
        return -1;
    }

    return -1;
}


/**
 * 以叶子节点为单位的查找索引
 * @param key
 * @return
 */
offset_t node_leaf::search(value_t& key)
{
    const int CLINT = tree.index.columnLength + 8;
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1);
    if (keyNum == 0) return offset_t(); //空块则返回null
    int start = 0, end = keyNum - 1, middle = 0;
    shared_ptr<value_t> middleKey = key.copy();

    while (start <= end)
    {
        middle = (start + end) / 2;
        middleKey->read(curBlock, 17 + middle * CLINT);

        if (key.compareTo(*middleKey) == 0)
            break;
        else if (key.compareTo(*middleKey) < 0)
            end = middle - 1;
        else
            start = middle + 1;
    }

    int pos = 9 + middle * CLINT;
    middleKey->read(curBlock, 8 + pos);

    // 将找到的位置上的表文件偏移信息存入off中，返回给上级调用
    // 再次确认有没有找到这个索引键值，没有则返回空集
    offset_t off;
    if (key.compareTo(*middleKey) == 0)
        off.emplace_back(curBlock.readInt(pos), curBlock.readInt(pos + 4));
    return off;
}


/**
 * 以叶子节点为单位的范围查找索引
 */
offset_t node_leaf::search(value_t& beginKey, value_t& endKey)
{
    const int CLINT = tree.index.columnLength + 8;
    block_t *curBlock = &getBlock();
    int keyNum = curBlock->readInt(1);
    if (keyNum == 0) return offset_t();

    auto key = beginKey.copy();
    auto ekey = endKey.copy();
    auto middleKey = beginKey.copy();
    int start = 0, end = keyNum - 1, middle = 0;

    while (start <= end)
    {
        middle = (start + end) / 2;
        middleKey->read(*curBlock, 17 + middle * CLINT);
        if (key->compareTo(*middleKey) == 0)
            break;
        if (key->compareTo(*middleKey) < 0)
            end = middle - 1;
        else
            start = middle + 1;
    }

    int pos = 9 + middle * CLINT;
    middleKey->read(*curBlock, 8 + pos);

    // 将找到的位置上的表文件偏移信息存入off中，返回给上级调用
    offset_t off;
    while (middleKey->compareTo(*ekey) <= 0)
    {
        if (middleKey->compareTo(*key) >= 0)
            off.emplace_back(curBlock->readInt(pos), curBlock->readInt(pos + 4));
        middle += 1;

        if (middle >= keyNum)
        {
            // 获取尾指针并读取新的块进来
            if (curBlock->readInt(9 + keyNum * CLINT) == '&' * SZSL) break;
            curBlock = &getBlock(curBlock->readInt(9 + keyNum * CLINT));
            keyNum = curBlock->readInt(1);
            middle = 0; pos = 9;
            middleKey->read(*curBlock, 8 + pos);
        }
        else
        {
            pos = 9 + middle * CLINT;
            middleKey->read(*curBlock, 8 + pos);
        }
    }

    return off;
}


/**
 * 以叶子节点为单位的块合并
 */
block_id node_leaf::merge(block_id _afterBlock)
{
    const int CLINT = tree.index.columnLength + 8;
    block afterBlock = getBlock(_afterBlock);
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1);
    int afterkeyNum = afterBlock.readInt(1);

    // 将after块的内容复制到this块的后面
    afterBlock.copyTo(
        9,
        curBlock,
        9 + keyNum * CLINT,
        PNT_LEN + afterkeyNum * CLINT);

    // 更新索引数量
    keyNum += afterkeyNum;
    curBlock.writeInt(1, keyNum);
    // TODO: 这个块真的是从末尾删除的吗？
    // tree.index.blockCount--;

    // 在父节点中删除这个被废弃的after块的信息(标号及它前面的路标)
    int parentBlockNum = curBlock.readInt(5);

    auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
    return parentNode->remove(_afterBlock);
}


/**
 * 从兄弟节点挪来一条索引进行重排
 * @param siblingBlock 兄弟节点在其后
 * @return
 */
shared_ptr<value_t> node_leaf::rearrangeAfter(block_id _siblingBlock)
{
    const int CLINT = tree.index.columnLength + 8;
    block siblingBlock = getBlock(_siblingBlock);
    block curBlock = getBlock();
    int siblingKeyNum = siblingBlock.readInt(1);
    int keyNum = curBlock.readInt(1);

    // 要挪过来的索引信息
    int blockOffset = siblingBlock.readInt(9);
    int offset = siblingBlock.readInt(13);
    auto key = tree.sampleVal->copy();
    key->read(siblingBlock, 17);

    //更新兄弟块的内容
    siblingKeyNum--;
    siblingBlock.writeInt(1, siblingKeyNum);
    siblingBlock.copyTo(
        9 + CLINT,
        siblingBlock,
        9,
        PNT_LEN + siblingKeyNum * CLINT);

    // this块和兄弟块之间的新路标
    auto changeKey = key->copy();
    changeKey->read(siblingBlock, 17);

    // 添加挪来的索引
    setKeydata(curBlock, 9 + keyNum * CLINT, *key, blockOffset, offset);
    keyNum++;
    curBlock.writeInt(1, keyNum);
    // 注意不要漏了链表信息(下一块的标号)
    curBlock.writeInt(9 + keyNum * CLINT, _siblingBlock);

    return changeKey;
}


/**
 * 兄弟节点在其前
 * @param siblingBlock
 * @return
 */
shared_ptr<value_t> node_leaf::rearrangeBefore(block_id _siblingBlock)
{
    block siblingBlock = getBlock(_siblingBlock);
    block curBlock = getBlock();
    int siblingKeyNum = siblingBlock.readInt(1);
    int keyNum = curBlock.readInt(1);
    const int CLINT = tree.index.columnLength + 8;

    siblingKeyNum--;
    siblingBlock.writeInt(1, siblingKeyNum);

    // 要挪来的索引信息
    int blockOffset = siblingBlock.readInt(9 + siblingKeyNum * CLINT);
    int offset = siblingBlock.readInt(13 + siblingKeyNum * CLINT);
    auto key = tree.sampleVal->copy();
    key->read(siblingBlock, 17 + siblingKeyNum * CLINT);

    // 保存好链表信息
    siblingBlock.writeInt(9 + siblingKeyNum * CLINT, this->curBlock);

    // 挪出新索引位置
    curBlock.copyTo(
        9,
        curBlock,
        9 + CLINT,
        PNT_LEN + keyNum * CLINT);

    setKeydata(curBlock, 9, *key, blockOffset, offset); //插入新索引
    keyNum++;
    curBlock.writeInt(1, keyNum);

    // 需要父块的更新成的新路标
    auto changeKey = key->copy();
    changeKey->read(curBlock, 17);
    return changeKey;
}
