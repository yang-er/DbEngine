#include "recording.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <io.h>
using std::sort;
using std::function;

memtable_t& recording_t::orderby(memtable_t &source, fieldref_t &field, bool inc)
{
    function<bool(const tuple &, const tuple &)> cmp;
    int i = field.fieldId;
    if (inc)
        cmp = [i] (const tuple &a, const tuple &b) { return a[i]->compareTo(*b[i]) < 0; };
    else
        cmp = [i] (const tuple &a, const tuple &b) { return b[i]->compareTo(*a[i]) < 0; };
    sort(source.tuples.begin(), source.tuples.end(), cmp);
    return source;
}

memtable_t recording_t::project(memtable_t &source, const vector<int> &rows)
{
    memtable_t retVal;
    for (auto i : rows)
        retVal.fields.push_back(source.fields[i]);

    for (auto j : source.tuples)
    {
        retVal.tuples.push_back(tuple());
        tuple &t = retVal.tuples.back();
        for (auto i : rows) t.push_back(j[i]);
    }

    return retVal;
}

memtable_t& recording_t::select(memtable_t &source, condition cond)
{
    for (auto it = source.tuples.begin(); it != source.tuples.end(); )
    {
        if (cond->evaluate(*it)) it++;
        else it = source.tuples.erase(it);
    }

    return source;
}

memtable_t recording_t::join(memtable_t &a, memtable_t &b)
{
    memtable_t retVal;
    retVal.fields.resize(a.fields.size() + b.fields.size());

    for (int i = 0; i < a.fields.size(); i++)
    {
        retVal.fields[i] = a.fields[i];
        if (!a.tmpName.empty())
            retVal.fields[i].name = a.tmpName + "." + a.fields[i].name;
    }

    for (int i = 0; i < b.fields.size(); i++)
    {
        retVal.fields[i + a.fields.size()] = b.fields[i];
        if (!b.tmpName.empty())
            retVal.fields[i + a.fields.size()].name = b.tmpName + "." + b.fields[i].name;
    }

    for (auto &ta : a.tuples)
    {
        for (auto &tb : b.tuples)
        {
            retVal.tuples.push_back(tuple());
            tuple &t = retVal.tuples.back();
            t.insert(t.end(), ta.begin(), ta.end());
            t.insert(t.end(), tb.begin(), tb.end());
        }
    }

    return retVal;
}

shared_ptr<tuple> recording_t::getTuple(const string &tableName, int tupleOffset)
{
    vector<int> os({tupleOffset});
    memtable_t t = getTuples(tableName, os);
    if (t.tuples.empty()) return nullptr;
    return std::make_shared<tuple>(t.tuples.front());
}

bool recording_t::checkUnique(table tbl, tuple &value)
{
    auto fn = tbl.getName() + ".table";
    const int tplen = tbl.getTupleLength();
    const int tpblock = block_t::SIZE / (4 + tplen);

    tuple tt;
    vector<int> usedFields;
    for (int k = 0; k < value.size(); k++)
    {
        tt.push_back(value[k]->copy());
        if (tbl.getField(k).unique)
            usedFields.push_back(k);
    }

    for (int i = 1, j = tbl.getTupleCount(); i <= j; i++)
    {
        int blockOffset = i / tpblock;
        block blk = bufferManager->getBlock(fn, blockOffset);
        int byteOffset = (4 + tplen) * (i % tpblock);
        if (blk.readInt(byteOffset) >= 0) continue;
        byteOffset += 4;
        for (auto uf : usedFields)
        {
            tt[uf]->read(blk, byteOffset + tbl.getField(uf).offset);
            if (tt[uf]->compareTo(*value[uf]) == 0) return false;
        }
    }

    return true;
}

memtable_t recording_t::getTuples(const string &tableName, vector<int> &tupleOffsets, bool verbose)
{
    table t = catalogManager->getTable(tableName);
    const int tplen = t.getTupleLength();
    const int tpblock = block_t::SIZE / (4 + tplen);
    memtable_t retVal;
    retVal.tmpName = tableName;
    for (int i = 0; i < t.getFieldLength(); i++)
        retVal.fields.push_back(t.getField(i));

    for (auto tupleOffset : tupleOffsets)
    {
        if (verbose)
            std::cerr << "Reading tuple @" << tupleOffset << std::endl;
        int blockOffset = tupleOffset / tpblock;
        block blk = bufferManager->getBlock(tableName + ".table", blockOffset);
        int byteOffset = (4 + tplen) * (tupleOffset % tpblock);
        if (blk.readInt(byteOffset) >= 0) continue;
        byteOffset += 4;
        retVal.tuples.emplace_back();
        tuple &tt = retVal.tuples.back();

        for (int i = 0; i < t.getFieldLength(); i++)
        {
            shared_ptr<value_t> val;
            field f = t.getField(i);
            if (f.type == 'I') val.reset(new value_int);
            else if (f.type == 'F') val.reset(new value_float);
            else if (f.type == 'S') val.reset(new value_string);
            val->read(blk, byteOffset + f.offset);
            tt.push_back(val);
        }
    }

    if (verbose)
        std::cerr << "Mission complete.\n" << std::endl;
    return retVal;
}

bool recording_t::createTable(const string &tableName)
{
    auto fn = tableName + ".table";
    if (!access(fn.data(), 0)) return false;
    block blk = bufferManager->getBlock(fn, 0);
    blk.writeInt(0, 0);
    return true;
}

bool recording_t::dropTable(const string &tableName)
{
    auto fn = tableName + ".table";
    if (access(fn.data(), 0)) return false;
    bufferManager->setInvalid(fn);
    return remove(fn.data()) == 0;
}

int recording_t::insert(const string &tableName, const tuple &row)
{
    table t = catalogManager->getTable(tableName);
    auto fn = tableName + ".table";
    const int tplen = t.getTupleLength();
    const int tpblock = block_t::SIZE / (4 + tplen);
    block blk1 = bufferManager->getBlock(fn, 0);
    int tupleOffset = blk1.readInt(0);

    if (tupleOffset > 0)
    {
        block blk2 = bufferManager->getBlock(fn, tupleOffset / tpblock);
        int nextTuple = blk2.readInt((4 + tplen) * (tupleOffset % tpblock));
        blk1.writeInt(0, nextTuple);
    }
    else
    {
        tupleOffset = 1 + t.getTupleCount();
        catalogManager->addTuple(tableName, 1);
    }

    block blk3 = bufferManager->getBlock(fn, tupleOffset / tpblock);
    int byteOffset = (4 + tplen) * (tupleOffset % tpblock);
    blk3.writeInt(byteOffset, -1);
    byteOffset += 4;
    for (int i = 0; i < t.getFieldLength(); i++)
        row[i]->write(blk3, byteOffset + t.getField(i).offset);
    for (auto &idx : t.indexs)
        indexManager->insertKey(idx.second, *row[idx.second.columnId], tupleOffset / tpblock, tupleOffset);
    return tupleOffset;
}

int recording_t::Delete(const string &tableName, condition cond)
{
    table t = catalogManager->getTable(tableName);
    auto fn = tableName + ".table";
    const int tplen = t.getTupleLength();
    const int tpblock = block_t::SIZE / (4 + tplen);

    int chained;
    {
        block b = bufferManager->getBlock(fn, 0);
        chained = b.readInt(0);
    }

    tuple tt;
    for (int k = 0; k < t.getFieldLength(); k++)
    {
        shared_ptr<value_t> val;
        field f = t.getField(k);
        if (f.type == 'I') val.reset(new value_int);
        else if (f.type == 'F') val.reset(new value_float);
        else if (f.type == 'S') val.reset(new value_string);
        tt.push_back(val);
    }

    int tupleOffset = 1, numdel = 0;
    for (int i = 0, j = t.getTupleCount(); i < j; i++, tupleOffset++)
    {
        int blockOffset = tupleOffset / tpblock;
        block blk = bufferManager->getBlock(tableName + ".table", blockOffset);
        int byteOffset = (4 + tplen) * (tupleOffset % tpblock);
        if (blk.readInt(byteOffset) >= 0) continue;
        byteOffset += 4;
        for (int k = 0; k < t.getFieldLength(); k++)
            tt[k]->read(blk, byteOffset + t.getField(k).offset);

        if (cond->evaluate(tt))
        {
            // this tuple is to delete.
            for (auto &idx : t.indexs)
                indexManager->deleteKey(idx.second, *tt[idx.second.columnId]);
            numdel++;
            blk.writeInt(byteOffset-4, chained);
            chained = tupleOffset;
        }
    }

    // and
    {
        block b = bufferManager->getBlock(fn, 0);
        b.writeInt(0, chained);
    }
    return numdel;
}

memtable_t recording_t::getAllTuples(const string &tableName, bool verbose)
{
    vector<int> all;
    table t = catalogManager->getTable(tableName);
    for (int i = 1; i <= t.getTupleCount(); i++)
        all.push_back(i);
    return getTuples(tableName, all, verbose);
}

int recording_t::update(const string &tableName, condition cond, map<int, shared_ptr<value_t>> &vp)
{
    table t = catalogManager->getTable(tableName);
    auto fn = tableName + ".table";
    const int tplen = t.getTupleLength();
    const int tpblock = block_t::SIZE / (4 + tplen);

    tuple tt;
    for (int k = 0; k < t.getFieldLength(); k++)
    {
        shared_ptr<value_t> val;
        field f = t.getField(k);
        if (f.type == 'I') val.reset(new value_int);
        else if (f.type == 'F') val.reset(new value_float);
        else if (f.type == 'S') val.reset(new value_string);
        tt.push_back(val);
    }

    int tupleOffset = 1, numupd = 0;
    for (int i = 0, j = t.getTupleCount(); i < j; i++, tupleOffset++)
    {
        int blockOffset = tupleOffset / tpblock;
        block blk = bufferManager->getBlock(tableName + ".table", blockOffset);
        int byteOffset = (4 + tplen) * (tupleOffset % tpblock);
        if (blk.readInt(byteOffset) >= 0) continue;
        byteOffset += 4;
        for (int k = 0; k < t.getFieldLength(); k++)
            tt[k]->read(blk, byteOffset + t.getField(k).offset);

        if (cond->evaluate(tt))
        {
            // this tuple is to update.
            numupd++;
            for (auto &v : vp)
                v.second->write(blk, byteOffset + t.fields[v.first].offset);
        }
    }

    return numupd;
}

shared_ptr<recording_t> recordingManager;
