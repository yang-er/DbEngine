#include "nodes.h"
#define MinLef tree.MIN_FOR_LEAF
#define MaxLef tree.MAX_FOR_LEAF

node_leaf::node_leaf(block_id blk, bptree& bpt) : node_t(blk, bpt)
{
    block curBlock = getBlock();
    curBlock.writeChar(0, 'L'); // ��ʶΪҶ�ӿ�
    curBlock.writeInt(1, 0); // ���ڹ���0��keyֵ
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
 * ��Ҷ�ӽڵ�Ϊ��λ�������Ĳ���
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

    if (++keyNum > MaxLef) // ���ѽڵ�
    {
        bool half = false;
        int newBlockOffset = tree.index.blockCount;
        tree.index.blockCount++;
        block newBlock = getBlock(newBlockOffset);
        auto newNode = std::make_shared<node_leaf>(newBlockOffset, tree);
        shared_ptr<value_t> tmp = key.copy();

        for (int i = 0; i < MinLef - 1; i++) // ����ԭ��
        {
            int pos = 17 + i * CLINT;
            tmp->read(curBlock, pos);
            if (key.compareTo(*tmp) >= 0) continue;

            curBlock.copyTo( // �ѵ�MinLef-1����ʼ�ļ�¼copy����block
                9 + (MinLef - 1) * CLINT,
                newBlock,
                9,
                PNT_LEN + (MaxLef - MinLef + 1) * CLINT
            );

            curBlock.copyTo( // ���²���������λ��
                9 + i * CLINT,
                curBlock,
                9 + (i + 1) * CLINT,
                PNT_LEN + (MinLef - 1 - i) * CLINT
            );

            // ����������
            setKeydata(curBlock, 9 + i * CLINT, key, blockOffset, offset);
            half = true;
            break;
        }

        if (!half) // �����¿�
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

                    // ����������
                    setKeydata(newBlock, 9 + i * CLINT, key, blockOffset, offset);
                    break;
                }
            }

            if (i == MaxLef - MinLef)
            {
                newBlock.copyTo( // ���²���������λ��
                    9 + i * CLINT,
                    newBlock,
                    9 + (i + 1) * CLINT,
                    PNT_LEN + (MaxLef - MinLef - i) * CLINT);

                // ����������
                setKeydata(newBlock, 9 + i * CLINT, key, blockOffset, offset);
            }
        }

        curBlock.writeInt(1, MinLef);
        newBlock.writeInt(1, MaxLef + 1 - MinLef);

        // ԭ����������¿�Ŀ�ţ�����������Ҷ�ӽڵ��һ������
        curBlock.writeInt(9 + MinLef * CLINT, newBlockOffset);

        int parentBlockNum;
        shared_ptr<node_internal> parentNode;

        if (curBlock.readInt(5) == '$' * SZSL) // û�и��ڵ㣬�򴴽����ڵ�
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
            newBlock.writeInt(5, parentBlockNum); // �½ڵ�ĸ���Ҳ���Ǿɽڵ�ĸ���
            parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
        }

        // ��branchKey(�¿�ĵ�һ��������ֵ)�ύ���ø�����ѳ�����������
        shared_ptr<value_t> branchKey = key.copy();
        branchKey->read(newBlock, 17);
        return parentNode->branchInsert(*branchKey, shared_from_this(), newNode);
    }
    else // ����Ҫ���ѽڵ�ʱ
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

            if (key.compareTo(*tmp) == 0) // ���м�ֵ�����
            {
                setKeydata(curBlock, 9 + i * CLINT, key, blockOffset, offset);
                return -1;
            }

            tmp->read(curBlock, pos);
            if (key.compareTo(*tmp) < 0) // �ҵ������λ��
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
 * ��Ҷ�ӽڵ�Ϊ��λ������ɾ��
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
        if (key.compareTo(*tmp) < 0) // û�ҵ�������ֵ
            return -1;
        if (key.compareTo(*tmp) > 0)
            continue;

        //�ҵ���Ӧ�ļ�ֵ
        curBlock.copyTo( //�Ƴ���������
            9 + (i + 1) * CLINT,
            curBlock,
            9 + i * CLINT,
            PNT_LEN + (keyNum - 1 - i) * CLINT);
        keyNum--;
        curBlock.writeInt(1, keyNum);

        if (keyNum >= MinLef) return -1; //��Ȼ��������Ҫ��
        if (curBlock.readInt(5) == '$' * SZSL) return -1;  // û�и��飬����Ϊ��

        bool lastFlag = false;
        if (curBlock.readInt(9 + keyNum * CLINT) == '&' * SZSL) lastFlag = true; // Ҷ�ӿ���������һ��

        int sibling = curBlock.readInt(9 + keyNum * CLINT);
        block_t* siblingBlock = &getBlock(sibling);
        int parentBlockNum = curBlock.readInt(5);

        // û���ҵ������ֵܽڵ�
        /* ��Ȼ�к����鵫����ͬһ�����׵��ֵܿ� */
        if (lastFlag || siblingBlock->readInt(5) != parentBlockNum)
        {
            // ͨ��������ǰ���ֵܿ�
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

            //���Ժϲ�
            if ((siblingBlock->readInt(1) + keyNum) <= MaxLef)
            {
                auto sbNode = std::make_shared<node_leaf>(sibling, tree, true);
                return sbNode->merge(this->curBlock);
            }

            // �����ڵ�ԭ������MIN_FOR_LEAF��deleteһ���Ժ�����û���ǣ����ʱ����漰�����ڵ�
            if (siblingBlock->readInt(1) == MinLef) return -1;

            // ����
            auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
            parentNode->exchange(*rearrangeBefore(sibling), sibling);
            return -1;
        }

        // �ϲ�
        if (siblingBlock->readInt(1) + keyNum <= MaxLef)
            return this->merge(sibling);

        // û���ǵ����
        if (siblingBlock->readInt(1) == MinLef) return -1;

        // ����
        auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
        parentNode->exchange(*rearrangeAfter(sibling), this->curBlock);
        return -1;
    }

    return -1;
}


/**
 * ��Ҷ�ӽڵ�Ϊ��λ�Ĳ�������
 * @param key
 * @return
 */
offset_t node_leaf::search(value_t& key)
{
    const int CLINT = tree.index.columnLength + 8;
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1);
    if (keyNum == 0) return offset_t(); //�տ��򷵻�null
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

    // ���ҵ���λ���ϵı��ļ�ƫ����Ϣ����off�У����ظ��ϼ�����
    // �ٴ�ȷ����û���ҵ����������ֵ��û���򷵻ؿռ�
    offset_t off;
    if (key.compareTo(*middleKey) == 0)
        off.emplace_back(curBlock.readInt(pos), curBlock.readInt(pos + 4));
    return off;
}


/**
 * ��Ҷ�ӽڵ�Ϊ��λ�ķ�Χ��������
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

    // ���ҵ���λ���ϵı��ļ�ƫ����Ϣ����off�У����ظ��ϼ�����
    offset_t off;
    while (middleKey->compareTo(*ekey) <= 0)
    {
        if (middleKey->compareTo(*key) >= 0)
            off.emplace_back(curBlock->readInt(pos), curBlock->readInt(pos + 4));
        middle += 1;

        if (middle >= keyNum)
        {
            // ��ȡβָ�벢��ȡ�µĿ����
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
 * ��Ҷ�ӽڵ�Ϊ��λ�Ŀ�ϲ�
 */
block_id node_leaf::merge(block_id _afterBlock)
{
    const int CLINT = tree.index.columnLength + 8;
    block afterBlock = getBlock(_afterBlock);
    block curBlock = getBlock();
    int keyNum = curBlock.readInt(1);
    int afterkeyNum = afterBlock.readInt(1);

    // ��after������ݸ��Ƶ�this��ĺ���
    afterBlock.copyTo(
        9,
        curBlock,
        9 + keyNum * CLINT,
        PNT_LEN + afterkeyNum * CLINT);

    // ������������
    keyNum += afterkeyNum;
    curBlock.writeInt(1, keyNum);
    // TODO: ���������Ǵ�ĩβɾ������
    // tree.index.blockCount--;

    // �ڸ��ڵ���ɾ�������������after�����Ϣ(��ż���ǰ���·��)
    int parentBlockNum = curBlock.readInt(5);

    auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree, true);
    return parentNode->remove(_afterBlock);
}


/**
 * ���ֵܽڵ�Ų��һ��������������
 * @param siblingBlock �ֵܽڵ������
 * @return
 */
shared_ptr<value_t> node_leaf::rearrangeAfter(block_id _siblingBlock)
{
    const int CLINT = tree.index.columnLength + 8;
    block siblingBlock = getBlock(_siblingBlock);
    block curBlock = getBlock();
    int siblingKeyNum = siblingBlock.readInt(1);
    int keyNum = curBlock.readInt(1);

    // ҪŲ������������Ϣ
    int blockOffset = siblingBlock.readInt(9);
    int offset = siblingBlock.readInt(13);
    auto key = tree.sampleVal->copy();
    key->read(siblingBlock, 17);

    //�����ֵܿ������
    siblingKeyNum--;
    siblingBlock.writeInt(1, siblingKeyNum);
    siblingBlock.copyTo(
        9 + CLINT,
        siblingBlock,
        9,
        PNT_LEN + siblingKeyNum * CLINT);

    // this����ֵܿ�֮�����·��
    auto changeKey = key->copy();
    changeKey->read(siblingBlock, 17);

    // ���Ų��������
    setKeydata(curBlock, 9 + keyNum * CLINT, *key, blockOffset, offset);
    keyNum++;
    curBlock.writeInt(1, keyNum);
    // ע�ⲻҪ©��������Ϣ(��һ��ı��)
    curBlock.writeInt(9 + keyNum * CLINT, _siblingBlock);

    return changeKey;
}


/**
 * �ֵܽڵ�����ǰ
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

    // ҪŲ����������Ϣ
    int blockOffset = siblingBlock.readInt(9 + siblingKeyNum * CLINT);
    int offset = siblingBlock.readInt(13 + siblingKeyNum * CLINT);
    auto key = tree.sampleVal->copy();
    key->read(siblingBlock, 17 + siblingKeyNum * CLINT);

    // �����������Ϣ
    siblingBlock.writeInt(9 + siblingKeyNum * CLINT, this->curBlock);

    // Ų��������λ��
    curBlock.copyTo(
        9,
        curBlock,
        9 + CLINT,
        PNT_LEN + keyNum * CLINT);

    setKeydata(curBlock, 9, *key, blockOffset, offset); //����������
    keyNum++;
    curBlock.writeInt(1, keyNum);

    // ��Ҫ����ĸ��³ɵ���·��
    auto changeKey = key->copy();
    changeKey->read(curBlock, 17);
    return changeKey;
}
