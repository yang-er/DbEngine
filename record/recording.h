#pragma once

#include "value.h"
#include <vector>
#include <memory>
#include <map>
#include "../syntax/expression.h"
#include "../storage/buffer.h"
#include "../catalog/catalog.h"
#include "../indexing/indexing.h"
using std::vector;
using std::shared_ptr;
using expression::fieldref_t;
using std::map;

class recording_t
{
public:
    memtable_t getTuples(const string &tableName, vector<int> &tupleOffsets, bool verbose = false);
    shared_ptr<tuple> getTuple(const string &tableName, int tupleOffset);
    memtable_t getAllTuples(const string& tableName, bool verbose = false);
    bool createTable(const string &tableName);
    bool dropTable(const string &tableName);
    int insert(const string &tableName, const tuple &row);
    int update(const string &tableName, condition cond, map<int, shared_ptr<value_t>>&);
    int Delete(const string &tableName, condition cond);
    memtable_t project(memtable_t &source, const vector<int> &rows);
    memtable_t &select(memtable_t &source, condition cond);
    memtable_t &orderby(memtable_t &source, fieldref_t& field, bool inc);
    memtable_t join(memtable_t &a, memtable_t &b);
    bool checkUnique(table tbl, tuple &value);
};

extern shared_ptr<recording_t> recordingManager;
