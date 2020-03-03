#include "nodes.h"
#define MinChildIn tree.MIN_CHILDREN_FOR_INTERNAL
#define MaxChildIn tree.MAX_CHILDREN_FOR_INTERNAL

node_internal::node_internal(block_id blk, bptree &bpt) : node_t(blk, bpt)
{
    block curBlock = getBlock();
    curBlock.writeChar(0, 'I'); // ��ʶΪ�м��
    curBlock.writeInt(1, 0); // ���ڹ���0��keyֵ
    curBlock.writeInt(5, SZSL * '$'); // ˵��û�и�����
}

node_internal::node_internal(block_id blk, bptree &bpt, bool) : node_t(blk, bpt)
{
}


/**
 * ���м�ڵ�Ϊ��λ�Ĳ���
 * @param key ��ֵ
 * @param blockOffset ���
 * @param offset ����ƫ��
 * @return ����Ŀ��
 */
block_id node_internal::insert(value_t &key, int blockOffset, int offset)
{
    block curBlock = getBlock();
    auto tmp = key.copy();
    int keyNum = curBlock.readInt(1), i = 0; // ��ȡ·����

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
 * ���м�ڵ�Ϊ��λ��ɾ��
 * @param key ��ֵ
 * @return ɾ���Ŀ��
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
    return nextNode->remove(key); //�ݹ�ɾ��
}


/**
 * �����������ʱ��Ҫ���м�ڵ���з�֧
 * @param branchKey Ҫ�²����·��
 * @param lson ��·������ӽڵ㣨Ҳ�����Ѿ����ڵĽڵ㣩
 * @param rson ��·������ӽڵ�
 * @return �¸���
 */
block_id node_internal::branchInsert(value_t& branchKey, const shared_ptr<node_t> &lson, const shared_ptr<node_t> &rson)
{
    block curBlock = getBlock();
    int keNum = curBlock.readInt(1); // ��ȡ·����
    const int INKEYSZ = PNT_LEN + branchKey.size();

    if (keNum == 0) // ȫ�½ڵ�
    {
        keNum++;
        curBlock.writeInt(1, keNum);
        branchKey.write(curBlock, 9 + PNT_LEN);
        curBlock.writeInt(9, lson->curBlock);
        curBlock.writeInt(9 + INKEYSZ, rson->curBlock);
        return this->curBlock; // ���ýڵ�鷵����Ϊ�¸���
    }
    else if (++keNum > MaxChildIn) //���ѽڵ�
    {
        // ��Ϊ���ܴ������ڴ�ռ䣬ֻ����
        // �����鷳�ķ����������ӿ����Щ·���ָ��
        bool half = false;

        // ����һ���¿鲢��װ
        int newBlockOffset = tree.index.blockCount;
        tree.index.blockCount++;

        block newBlock = getBlock(newBlockOffset);
        auto newNode = std::make_shared<node_internal>(newBlockOffset, tree);

        // ����֪���²���·��ʹ���������ޣ�Ҳ����
        // ����·��ΪMAX+1����������Ϊԭ���Ŀ�
        // ����MIN��·�꣬�����¿��Ŀ���MAX+1-MIN��·��
        curBlock.writeInt(1, MinChildIn);
        newBlock.writeInt(1, MaxChildIn + 1 - MinChildIn);

        // ������·�������ԭ���Ŀ�Ҳ������MIN֮��
        for (int i = 0; i < MinChildIn; i++)
        {
            int pos = 9 + PNT_LEN + i * (branchKey.size() + PNT_LEN);
            shared_ptr<value_t> tmp = branchKey.copy();
            tmp->read(curBlock, pos);

            // �ҵ���·������λ��
            if (branchKey.compareTo(*tmp) >= 0) continue;

            curBlock.copyTo(
                // �ѵ�MIN_CHILDREN_FOR_INTERNA����ʼ�ļ�¼copy����block
                9 + MinChildIn * INKEYSZ,
                newBlock,
                9,
                PNT_LEN + (MaxChildIn - MinChildIn) * INKEYSZ);

            curBlock.copyTo( // ���²���������λ��
                9 + PNT_LEN + i * INKEYSZ,
                curBlock,
                9 + PNT_LEN + (i + 1) * INKEYSZ,
                // ע�⻹Ҫ��ԭ������һ��·�걣����������������Ҫ��Ϊ������²���·�꣩
                (MinChildIn - i) * INKEYSZ - PNT_LEN);

            // �����²�����Ϣ
            branchKey.write(curBlock, 9 + PNT_LEN + i * INKEYSZ);
            curBlock.writeInt(9 + (i+1) * INKEYSZ, rson->curBlock);
            half = true;
            break;
        }

        // ��·��������¿��Ŀ�Ҳ��������λ�ó�����MIN
        if (!half)
        {
            //�ѵ�MIN+1����ʼ�ļ�¼copy����block
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

                // ���²���������λ��
                newBlock.copyTo(
                    9 + PNT_LEN + i * INKEYSZ,
                    newBlock,
                    9 + PNT_LEN + (i + 1) * INKEYSZ,
                    (MaxChildIn - MinChildIn - 1 - i) * INKEYSZ
                );

                //�����²�����Ϣ
                branchKey.write(newBlock, 9 + PNT_LEN + i * INKEYSZ);
                newBlock.writeInt(9 + (i+1) * INKEYSZ, rson->curBlock);
                break;
            }
        }

        // �ҳ�ԭ�����¿�֮���·�꣬�ṩ�����ڵ���������
        shared_ptr<value_t> spiltKey = branchKey.copy();
        spiltKey->read(curBlock, 9 + PNT_LEN + MinChildIn * INKEYSZ);

        newBlock.setPin(true);
        curBlock.setPin(true);

        //�����¿���ӿ�ĸ���
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
        if (curBlock.readInt(5) == SZSL * '$') // û�и��ڵ㣬�򴴽����ڵ�
        {
            // �����¿鲢��װ
            parentBlockNum = tree.index.blockCount;
            block parentBlock = getBlock(parentBlockNum);
            tree.index.blockCount++;

            // ���ø�����Ϣ
            curBlock.writeInt(5, parentBlockNum);
            newBlock.writeInt(5, parentBlockNum);
        }
        else
        {
            // �¿�ĸ���Ҳ���Ǿɿ�ĸ���
            parentBlockNum = curBlock.readInt(5);
            newBlock.writeInt(5, parentBlockNum);
        }

        auto parentNode = std::make_shared<node_internal>(parentBlockNum, tree);
        // �������׿鲢Ϊ���װ��Ȼ���ٵݹ����
        return parentNode->branchInsert(*spiltKey, shared_from_this(), newNode);
    }
    else // ����Ҫ���ѽڵ�ʱ
    {
        auto tmp = branchKey.copy(); int i;
        for (i = 0; i < keNum - 1; i++)
        {
            tmp->read(curBlock, 9 + PNT_LEN + i * INKEYSZ);
            if (branchKey.compareTo(*tmp) >= 0) continue;

            //�ҵ������λ��
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
 * ɾ�������в����Ľڵ�ϲ���this���after���Լ�����֮���unionKey
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

    // ����unionKey
    unionKey.write(curBlock, 9 + keyNum * INKEYSZ + PNT_LEN);

    // ���¼���·���С��ԭ·����+after·����+unionKey��
    keyNum = keyNum + afterkeyNum + 1;
    curBlock.writeInt(1, keyNum);

    // �ҵ�����
    int parentBlockNum = curBlock.readInt(5);
    auto parent = std::make_shared<node_internal>(parentBlockNum, tree, true);
    // TODO: �����һ���Ǵ�ĩβɾ������
    // tree.index.blockCount--;

    // �ڸ�����ɾ��after��
    return parent->remove(_afterBlock);
}


/**
 * ɾ�������в������ֵܿ��������ţ�
 * this���after���Լ�����֮���internalKey��
 * ���ص�changeKey��Ϊ�˸��¸�����������ָ���м�ļ�ֵ
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

    // Ҫת�Ƶ�һ��ָ������
    int blockOffset = siblingBlock.readInt(9);
    // ��internalKey���ֵܿ�ĵ�һ��ָ�븴�Ƶ�this���β����·������1
    internalKey.write(curBlock, 9 + PNT_LEN + keyNum * INKEYSZ);
    curBlock.writeInt(9 + (keyNum + 1) * INKEYSZ, blockOffset);
    keyNum++;
    curBlock.writeInt(1, keyNum);

    // �ֵܿ��·������1����ȡ�ֵܿ�ĵ�һ����ֵ
    // ��Ϊ���¸���ļ�ֵ���ٽ��ֵܿ�������Ϣ
    // ��ǰŲһ��ָ���·��ľ���
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
 * �ֵܽڵ�����ǰ
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

    // �ֵܿ�����һ��·����Ϊ����ĸ���·�꣬���һ��ָ�뽫ת�Ƹ�this��
    shared_ptr<value_t> changeKey = internalKey.copy();
    changeKey->read(siblingBlock, 9 + PNT_LEN + siblingKeyNum * INKEYSZ);
    int blockOffset = siblingBlock.readInt(9 + (siblingKeyNum + 1) * INKEYSZ);

    // ����ָ���·���ó�λ��
    curBlock.copyTo(
        9,
        curBlock,
        9 + INKEYSZ,
        PNT_LEN + keyNum * INKEYSZ);

    curBlock.writeInt(9, blockOffset);
    // ������ֵܿ�Ų��������ָ��
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

        if (ptr == blk) // ����ҵ����ӿ���
        {
            curBlock.copyTo(
                9 + PNT_LEN + (i - 1) * INKEYSZ,
                curBlock,
                9 + PNT_LEN + i * INKEYSZ,
                (keyNum - i) * INKEYSZ);

            keyNum--;
            curBlock.writeInt(1, keyNum);

            if (keyNum >= MinChildIn) return -1;
            //�Ƴ���ֱ�ӽ���

            // û�и��ڵ�ʱ
            if (curBlock.readInt(5) == SZSL * '$')
            {
                if (keyNum == 0)
                {
                    //û��·�ֻ꣬��һ���ӿ���ʱ���������ӿ���Ϊ���飬��this��ɾ��
                    // TODO: tree.index.blockCount--;
                    return curBlock.readInt(9);
                }

                return -1;
            }

            // �ҵ����׿�
            int parentBlockNum = curBlock.readInt(5);
            block parentBlock = getBlock(parentBlockNum);
            int parentKeyNum = parentBlock.readInt(1);

            // ���Һ����ֵܿ�
            int sibling, j = 0;
            for (; j < parentKeyNum; j++)
            {
                int ppos = 9 + j * INKEYSZ;
                if (this->curBlock == parentBlock.readInt(ppos))
                {
                    // ���������ֵܿ�
                    sibling = parentBlock.readInt(ppos + INKEYSZ);
                    block siblingBlock = getBlock(sibling);
                    auto unionKey = tree.sampleVal->copy();
                    unionKey->read(parentBlock, ppos + PNT_LEN);

                    // �ܹ��ϲ�
                    if (siblingBlock.readInt(1) + keyNum <= MaxChildIn)
                        return merge(*unionKey, sibling);

                    // �����ڵ�ԭ������MIN_FOR_LEAF��deleteһ���Ժ�����û���ǣ����ʱ����漰�����ڵ�
                    // �������û�д���Ҫ����Ļ����漰��4���ֵܿ飬��Ϊthis����MIN-1��·�꣬
                    // �ֵܿ���MIN��·�꣬���ֵܿ�Ųһ��·����Ȼ�ǰ׷���������������ȴҲ��һ���ܹ��ϲ���
                    // ���Ժ��鷳
                    if (siblingBlock.readInt(1) == MinChildIn) return -1;

                    // ���ܺϲ���ͨ�����ֵܿ�ת��һ��·��������B+��·����С����
                    auto parent = std::make_shared<node_internal>(parentBlockNum, tree, true);
                    parent->exchange(*rearrangeAfter(sibling, *unionKey), this->curBlock);
                    return -1;
                }
            }

            //�Ҳ������飬ֻ����ǰ���ֵܿ�
            sibling = parentBlock.readInt(9 + (parentKeyNum - 1) * INKEYSZ);
            block siblingBlock = getBlock(sibling);
            auto unionKey = tree.sampleVal->copy();
            unionKey->read(parentBlock, 9 + (parentKeyNum - 1) * INKEYSZ + PNT_LEN);

            // �ܹ��ϲ�
            if (siblingBlock.readInt(1) + keyNum <= MaxChildIn)
            {
                auto s = std::make_shared<node_internal>(sibling, tree, true);
                return s->merge(*unionKey, this->curBlock);
            }

            //û�п��ǵ����
            if (siblingBlock.readInt(1) == MinChildIn) return -1;

            //���ŵ����
            auto p = std::make_shared<node_internal>(parentBlockNum, tree, true);
            p->exchange(*rearrangeBefore(sibling, *unionKey), sibling);
            return -1;
        }
    }

    return -1;
}


/**
 * ���м�ڵ�Ϊ��λ�Ĳ���
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
    return nextNode->search(key); // �ݹ����
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
    return nextNode->search(begin, end); // �ݹ����
}
