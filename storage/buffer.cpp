#include "buffer.h"
#include <cstring>

buffer_t::buffer_t()
{
    memoryArea = new char[MAXMEMBLK * block_t::SIZE];
    memset(memoryArea, 0, MAXMEMBLK * block_t::SIZE);
    memset(L, -1, sizeof(L));
    memset(R, -1, sizeof(R));
    L[0] = R[0] = 0;
}

buffer_t::~buffer_t()
{
    for (int i = 1; i <= MAXMEMBLK; i++)
    {
        block t = blks[i];
        if (t.dirty) t.flushToFile();
    }
}

block buffer_t::getBlock(const string &file, int id)
{
    int i; bool needRead = true;

    if (mp.count(make_pair(file, id)))
    {
        i = mp[make_pair(file, id)];
        int ll = L[i], rr = R[i];
        L[rr] = ll, R[ll] = rr, R[i] = L[i] = -1;
        needRead = false;
    }
    else if (mp.size() >= MAXMEMBLK)
    {
        i = L[0];
        while (i && blks[i].pinned) i = L[i];
        mp.erase(make_pair(blks[i].fileName, blks[i].blockId));
        mp[make_pair(file, id)] = i;
        int ll = L[i], rr = R[i];
        L[rr] = ll, R[ll] = rr, R[i] = L[i] = -1;

        if (blks[i].dirty) blks[i].flushToFile();
    }
    else
    {
        i = mp.size()+1;
        mp[make_pair(file, id)] = i;
    }

    if (needRead)
    {
        char* blockmem = memoryArea + block_t::SIZE * (i-1);
        blks[i].slideTo(file, id, blockmem);
    }

    L[R[0]] = i, R[i] = R[0], R[0] = i, L[i] = 0;
    return blks[i];
}

void buffer_t::setInvalid(const string &file)
{
    for (int i = R[0]; i;)
    {
        if (blks[i].fileName != file)
        {
            i = R[i];
        }
        else
        {
            int ll = L[i], rr = R[i];
            L[rr] = ll, R[ll] = rr, R[i] = L[i] = -1;
            mp.erase(make_pair(blks[i].fileName, blks[i].blockId));
            memset(blks[i].data, 0, block_t::SIZE);
            blks[i].data = nullptr;
            blks[i].blockId = -1;
            blks[i].fileName = "";
            blks[i].dirty = false;
            blks[i].pinned = false;
            i = rr;
        }
    }
}

shared_ptr<buffer_t> bufferManager;
