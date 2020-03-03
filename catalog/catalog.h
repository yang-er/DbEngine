#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "table.h"
#include "index.h"
using std::string;
using std::unordered_map;
using std::shared_ptr;

class catalog_t
{
private:
    unordered_map<string, table_t> tables;
    unordered_map<string, string> indexs;

public:
    enum status
    {
        SUCCESS,
        TABLE_NOT_EXISTS,
        INDEX_NOT_EXISTS,
        TABLE_EXISTS,
        INDEX_EXISTS,
        INFO_CONFLICT,
    };

    table getTable(const string& tableName);
    index getIndex(const string& indexName);
    field getPrimaryKey(const string& tableName);
    field getField(const string& tableName, int i);
    field getField(const string& tableName, const string& fieldName);
    bool isTableExists(const string& tableName);
    bool isIndexExists(const string& indexName);
    bool isFieldExists(const string& tableName, const string& fieldName);
    string getIndexName(const string& tableName, const string& fieldName);
    status addTuple(const string& tableName, int count = 1);
    status deleteTuple(const string& tableName, int count);
    status updateIndex(const string& indexName, index indexInfo);
    status createTable(table newTable);
    status dropTable(const string& tableName);
    status createIndex(index newIndex);
    status dropIndex(const string& indexName);

    void showTables();
    void showIndexs();
    void load();
    void store();
};

extern shared_ptr<catalog_t> catalogManager;
