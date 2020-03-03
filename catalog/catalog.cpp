#define _CRT_SECURE_NO_WARNINGS
#include "catalog.h"
#include <cassert>
#include <io.h>
#include <iostream>
using std::cout;
using std::endl;
using status = catalog_t::status;
field_t NO_FIELD("", 'S', false);

status catalog_t::addTuple(const string &tableName, int count)
{
    if (!tables.count(tableName))
        return TABLE_NOT_EXISTS;
    tables[tableName].tupleCount += count;
    return SUCCESS;
}

status catalog_t::deleteTuple(const string &tableName, int count)
{
    if (!tables.count(tableName))
        return TABLE_NOT_EXISTS;
    tables[tableName].tupleCount += count;
    return SUCCESS;
}

status catalog_t::updateIndex(const string &indexName, index indexInfo)
{
    if (!indexs.count(indexName))
        return INDEX_NOT_EXISTS;
    if (!tables.count(indexInfo.tableName))
        return TABLE_NOT_EXISTS;
    if (indexs[indexName] != indexInfo.tableName)
        return INFO_CONFLICT;

    index oldIndex = tables[indexs[indexName]].indexs[indexName];
    if (oldIndex.fieldName != indexInfo.fieldName)
        return INFO_CONFLICT;
    oldIndex = indexInfo;
    return SUCCESS;
}

status catalog_t::createTable(table newTable)
{
    if (tables.count(newTable.name))
        return TABLE_EXISTS;
    tables[newTable.name] = newTable;
    return SUCCESS;
}

status catalog_t::dropTable(const string &tableName)
{
    if (!tables.count(tableName))
        return TABLE_NOT_EXISTS;
    table_t droppedTable = tables[tableName];
    tables.erase(tableName);
    for (auto &i : droppedTable.indexs)
        indexs.erase(i.first);
    return SUCCESS;
}

status catalog_t::createIndex(index newIndex)
{
    if (!tables.count(newIndex.tableName))
        return TABLE_NOT_EXISTS;
    if (indexs.count(newIndex.indexName))
        return INDEX_EXISTS;
    table now = tables[newIndex.tableName];
    now.indexs[newIndex.indexName] = newIndex;
    indexs[newIndex.indexName] = now.name;
    return SUCCESS;
}

status catalog_t::dropIndex(const string &indexName)
{
    if (!indexs.count(indexName))
        return INDEX_NOT_EXISTS;
    table now = tables[indexs[indexName]];
    now.indexs.erase(indexName);
    indexs.erase(indexName);
    return SUCCESS;
}

table catalog_t::getTable(const string &tableName)
{
    return tables[tableName];
}

index catalog_t::getIndex(const string &indexName)
{
    return tables[indexs[indexName]].indexs[indexName];
}

field catalog_t::getField(const string &tableName, int i)
{
    table now = tables[tableName];
    return now.fields[i];
}

field catalog_t::getField(const string &tableName, const string &fieldName)
{
    table now = tables[tableName];
    for (auto &i : now.fields)
        if (i.name == fieldName)
            return i;
    return NO_FIELD;
}

field catalog_t::getPrimaryKey(const string &tableName)
{
    table now = tables[tableName];
    return now.fields[now.primaryKey];
}

bool catalog_t::isFieldExists(const string &tableName, const string &fieldName)
{
    if (!tables.count(tableName))
        return false;
    table now = tables[tableName];
    for (auto &field : now.fields)
        if (field.name == fieldName)
            return true;
    return false;
}

bool catalog_t::isIndexExists(const string &indexName)
{
    return indexs.count(indexName);
}

bool catalog_t::isTableExists(const string &tableName)
{
    return tables.count(tableName);
}

string catalog_t::getIndexName(const string &tableName, const string &fieldName)
{
    if (!tables.count(tableName)) return "";
    table now = tables[tableName];
    for (auto &index : now.indexs)
        if (index.second.fieldName == fieldName)
            return index.first;
    return "";
}

void catalog_t::showTables()
{
    cout << tables.size() << " tables in database." << endl
         << endl
         << "----------" << endl << endl;

    int tot = 0;
    for (auto &t : tables)
    {
        table tt = t.second;

        cout << "TABLE " << ++tot << endl
             << "  Name: " << tt.name << endl
             << "  Columns: " << tt.fields.size() << endl
             << "  Primary key: " << tt.fields[tt.primaryKey].name << endl
             << "  Rows: " << tt.tupleCount << endl
             << "  Fields: " << endl;
        for (auto &f : tt.fields)
            cout << "    " << f.name << " " << f.type << (f.unique ? "U" : "") << endl;
        cout << endl
             << "----------" << endl << endl;
    }
}

void catalog_t::showIndexs()
{
    cout << indexs.size() << " indexes in database." << endl
         << endl;

    for (auto &i : indexs)
    {
        index id = tables[i.second].indexs[i.first];
        cout << "INDEX " << id.indexName << " ON " << id.tableName << "." << id.fieldName << endl;
    }

    cout << endl;
}

void catalog_t::load()
{
    FILE* fp = fopen("catalog.system", "rb+");
    if (fp == nullptr) return;
    int tot = 0;
    fread(&tot, sizeof(int), 1, fp);

    for (int i = 0; i < tot; i++)
    {
        table_t tmp = table_t::deserialize(fp);
        tables[tmp.name] = tmp;
        for (auto &i : tmp.indexs)
            indexs[i.first] = tmp.name;
    }

    fclose(fp);
}

void catalog_t::store()
{
    FILE* fp = fopen("catalog.system", "wb+");
    int tot = tables.size();
    fwrite(&tot, sizeof(int), 1, fp);
    for (auto &t : tables)
        t.second.serialize(fp);
    fclose(fp);
}

shared_ptr<catalog_t> catalogManager;
