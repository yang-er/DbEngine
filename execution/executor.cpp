#include "executor.h"
#include "../catalog/catalog.h"
#include "../syntax/parser.h"
#include "../indexing/indexing.h"
#include "../storage/buffer.h"
#include "../record/recording.h"
#include <io.h>
#include <iostream>
#include <fstream>
#include <algorithm>
using std::cerr;
using std::endl;
using std::cout;
using std::ifstream;

void executor::showCatalog(show_catalog_t *exp)
{
    if (exp->showTable)
        catalogManager->showTables();
    if (exp->showIndex)
        catalogManager->showIndexs();
}

bool executor::isEnded() const
{
    return ended;
}

void executor::quit(quit_t *exp)
{
    ended = true;
}

void executor::execfile(execfile_t *exp)
{
    ifstream ifs;
    ifs.open(exp->fileName);

    if (!ifs)
    {
        cerr << "Error loading file " << exp->fileName << "." << endl << endl;
        return;
    }

    tokenize::lexer reader(ifs);
    parser inp(reader);
    inp.process(this, false);
}

void executor::dropIndex(drop_index_t *exp)
{
    if (!catalogManager->isIndexExists(exp->indexName))
    {
        cerr << "Index " + exp->indexName + " not found." << endl << endl;
        return;
    }

    indexManager->dropIndex(catalogManager->getIndex(exp->indexName));
    catalogManager->dropIndex(exp->indexName);
    remove((exp->indexName + ".index").data());
    cerr << "Index " << exp->indexName << " was dropped." << endl << endl;
}

void executor::dropTable(drop_table_t *exp)
{
    if (!catalogManager->isTableExists(exp->tableName))
    {
        cerr << "Table " + exp->tableName + " not found." << endl << endl;
        return;
    }

    table tbl = catalogManager->getTable(exp->tableName);
    for (auto& idx : tbl.indexs)
    {
        indexManager->dropIndex(idx.second);
        remove((idx.first + ".index").data());
    }

    recordingManager->dropTable(exp->tableName);
    catalogManager->dropTable(exp->tableName);
    cerr << "Table " << exp->tableName << " was dropped." << endl << endl;
}

void executor::createIndex(create_index_t *exp)
{
    if (catalogManager->isIndexExists(exp->indexName))
    {
        cerr << "Index " + exp->indexName + " existed." << endl << endl;
        return;
    }

    if (!catalogManager->isTableExists(exp->tableName))
    {
        cerr << "Table " + exp->tableName + " not found." << endl << endl;
        return;
    }

    table t = catalogManager->getTable(exp->tableName);
    int fid = -1;
    for (int i = 0; i < t.getFieldLength(); i++)
        if (t.getField(i).name == exp->fieldName)
            fid = i;

    if (fid == -1)
    {
        cerr << "Field " + exp->fieldName + " not found." << endl << endl;
        return;
    }

    if (!t.fields[fid].unique)
    {
        cerr << "Field " + exp->fieldName + " should be unique." << endl << endl;
        return;
    }

    remove((exp->indexName + ".index").data());
    index_t op(exp->indexName, exp->tableName, t.getField(fid), fid, 0, 0);
    catalogManager->createIndex(op);
    indexManager->createIndex(op);

    if (catalogManager->isIndexExists(exp->indexName))
    {
        cerr << "Index " + exp->indexName + " has been established." << endl << endl;
        return;
    }
}

void executor::insertInto(insert_into_t* exp)
{
    if (!catalogManager->isTableExists(exp->tableName))
    {
        cerr << "Table " + exp->tableName + " not existed." << endl << endl;
        return;
    }

    table t = catalogManager->getTable(exp->tableName);

    if (t.getFieldLength() != exp->tuple.size())
    {
        cerr << "Too many or too few values." << endl << endl;
        return;
    }

    for (int i = 0; i < t.getFieldLength(); i++)
    {
        auto f = t.getField(i);
        shared_ptr<value_t> &val = exp->tuple[i];
        if (f.type == 'F' && val->type == 'I')
            val.reset(new value_float(std::static_pointer_cast<value_int>(val)->value));

        if (f.type != val->type)
        {
            cerr << "Value type for field '" << f.name << "' mismatch." << endl << endl;
            return;
        }
    }

    if (!recordingManager->checkUnique(t, exp->tuple))
    {
        cerr << "Unique check failed." << endl << endl;
        return;
    }

    recordingManager->insert(exp->tableName, exp->tuple);
    cerr << "Query finished, 1 row inserted." << endl << endl;
}

void executor::createTable(create_table_t* exp)
{
    if (catalogManager->isTableExists(exp->tableName))
    {
        cerr << "Table " + exp->tableName + " existed." << endl << endl;
        return;
    }

    if (exp->primaryKey == -1)
    {
        cerr << "Table " + exp->tableName + " has no primary Key." << endl << endl;
        return;
    }

    table_t t(exp->tableName, exp->fields[exp->primaryKey].name, exp->fields);
    catalogManager->createTable(t);
    index_t i("FK_" + exp->tableName + "_" + exp->fields[exp->primaryKey].name,
            exp->tableName,exp->fields[exp->primaryKey], exp->primaryKey);
    catalogManager->createIndex(i);

    if (!recordingManager->createTable(exp->tableName))
    {
        cerr << "Creation failed." << endl << endl;
        return;
    }

    if (!indexManager->createIndex(catalogManager->getIndex(i.indexName)))
    {
        cerr << "Creation failed." << endl << endl;
        return;
    }

    cerr << "Table " << exp->tableName << " created." << endl << endl;
}

void executor::deleteFrom(delete_t *exp)
{
    if (!catalogManager->isTableExists(exp->tableName))
    {
        cerr << "Table " + exp->tableName + " not exists." << endl << endl;
        return;
    }

    if (exp->where == nullptr)
    {
        exp->where.reset(new expression::from_result_t(true));
    }
    else
    {
        table t = catalogManager->getTable(exp->tableName);

        vector<shared_ptr<fieldref_t>> refs;
        exp->where->getFieldRef(refs);

        map<string, int> fieldRev;
        for (int i = 0, j = t.getFieldLength(); i < j; i++)
            fieldRev[t.getField(i).name] = i;

        for (auto &ref : refs)
        {
            if (fieldRev.count(ref->tempName))
            {
                ref->fieldId = fieldRev[ref->tempName];
            }
            else
            {
                cerr << "Field " + ref->tempName + " not exists." << endl << endl;
                return;
            }
        }
    }

    int count = recordingManager->Delete(exp->tableName, exp->where);
    cerr << "Query finished, " << count << " rows affected." << endl << endl;
}

void executor::selectFrom(select_t *exp)
{
    map<string, int> fieldRev;
    int fieldOffset = 0;

    for (auto &tn : exp->tableNames)
    {
        if (!catalogManager->isTableExists(tn))
        {
            cerr << "Table " + tn + " not exists." << endl << endl;
            return;
        }

        table t = catalogManager->getTable(tn);
        if (exp->tableNames.size() > 1)
            for (int i = 0; i < t.getFieldLength(); i++)
                fieldRev[t.getName() + "." + t.getField(i).name] = fieldOffset + i;
        else
            for (int i = 0; i < t.getFieldLength(); i++)
                fieldRev[t.getField(i).name] = i;
        fieldOffset += t.getFieldLength();
    }

    if (exp->where == nullptr)
    {
        exp->where.reset(new expression::from_result_t(true));
    }
    else
    {
        vector<shared_ptr<fieldref_t>> refs;
        exp->where->getFieldRef(refs);

        for (auto &ref : refs)
        {
            if (fieldRev.count(ref->tempName))
            {
                ref->fieldId = fieldRev[ref->tempName];
            }
            else
            {
                cerr << "Field " + ref->tempName + " not exists." << endl << endl;
                return;
            }
        }
    }

    vector<int> rows;
    for (auto &tl : exp->projection)
    {
        if (fieldRev.count(tl))
        {
            rows.push_back(fieldRev[tl]);
        }
        else
        {
            cerr << "Field " + tl + " not exists." << endl << endl;
            return;
        }
    }

    if (!exp->orderBy.empty() && !fieldRev.count(exp->orderBy))
    {
        cerr << "Field " + exp->orderBy + " not exists." << endl << endl;
        return;
    }

    memtable_t tbl;

    // ------------ MULTI TABLE QUERY ----------- //

    if (exp->tableNames.size() > 1)
    {
        tbl.tuples.emplace_back();
        for (auto &fn : exp->tableNames)
        {
            auto tmp = recordingManager->getAllTuples(fn, exp->verbose);
            tbl = recordingManager->join(tbl, tmp);
        }
    }

    // ------------ SINGLE TABLE QUERY ----------- //

    else
    {
        auto tn = exp->tableNames[0];
        table t = catalogManager->getTable(tn);
        auto range = exp->where->getPkeyRange(t.primaryKey);
        string idxName = catalogManager->getIndexName(t.getName(), t.fields[t.primaryKey].name);
        bool empty = false;
        vector<int> tupleOffsets;

        if (t.fields[t.primaryKey].type != 'I' || idxName.empty())
        {
            tupleOffsets.resize(t.getTupleCount());
            for (int i = 1; i <= t.getTupleCount(); i++)
                tupleOffsets[i-1] = i;
        }
        else if (range.first == INT_MAX && range.second == INT_MIN)
        {
            empty = true; // 无效空表
        }
        else
        {
            value_int lkey(range.first), rkey(range.second);
            index idx = catalogManager->getIndex(idxName);
            tupleOffsets = indexManager->searchRange(idx, lkey, rkey);
        }

        for (auto &idxOthers : t.indexs)
        {
            if (idxOthers.second.columnType != 'I') continue;
            if (idxOthers.second.columnId == t.primaryKey) continue;
            auto range2 = exp->where->getPkeyRange(idxOthers.second.columnId);

            if (range.first == INT_MAX && range.second == INT_MIN)
            {
                empty = true; // 无效空表
            }
            else if (range.first == INT_MIN && range.second == INT_MAX)
            {
                // 全部搜索，无需读取索引
            }
            else
            {
                value_int lkey(range2.first), rkey(range2.second);
                auto tupleOffsets2 = indexManager->searchRange(idxOthers.second, lkey, rkey);
                for (auto it = tupleOffsets.begin(); it != tupleOffsets.end(); )
                {
                    bool ok = false;

                    for (auto i : tupleOffsets2)
                    {
                        if (i != *it) continue;
                        ok = true;
                        break;
                    }

                    if (ok) it++;
                    else it = tupleOffsets.erase(it);
                }
            }
        }

        if (empty)
        {
            tbl.fields = t.fields;
        }
        else
        {
            tbl = recordingManager->getTuples(tn, tupleOffsets, exp->verbose);
        }
    }

    recordingManager->select(tbl, exp->where);

    if (!exp->orderBy.empty())
    {
        fieldref_t ref(tbl, fieldRev[exp->orderBy]);
        recordingManager->orderby(tbl, ref, exp->orderByAsc);
    }

    if (!rows.empty())
        tbl = recordingManager->project(tbl, rows);

    // ------------ RESULT DISPLAY ----------- //
    vector<int> lens;
    for (auto &f : tbl.fields)
        lens.push_back(f.name.size());

    for (auto &row : tbl.tuples)
    {
        for (int i = 0; i < lens.size(); i++)
        {
            if (row[i]->type != 'S')
                row[i].reset(new value_string(row[i]->to_string()));
            lens[i] = std::max(lens[i], (int)row[i]->to_string().size());
        }
    }

    int tolen = lens.size()+1;
    for (int i = 0; i < lens.size(); i++)
        tolen += lens[i] + 2;
    char linebuf[4096];
    memset(linebuf, '-', tolen);
    linebuf[tolen] = 0;
    cout << linebuf << endl;
    cout << "|";
    for (int i = 0; i < lens.size(); i++)
    {
        cout << " " << tbl.fields[i].name;
        for (int j = tbl.fields[i].name.size(); j < lens[i]; j++) cout << ' ';
        cout << " |";
    }
    cout << endl << linebuf << endl;

    for (auto &row : tbl.tuples)
    {
        cout << "|";
        for (int i = 0; i < lens.size(); i++)
        {
            cout << " " << row[i]->to_string();
            for (int j = row[i]->to_string().size(); j < lens[i]; j++) cout << ' ';
            cout << " |";
        }
        cout << endl;
    }

    cout << linebuf << endl << endl;
    cerr << "Query finished, " << tbl.tuples.size() << " rows selected." << endl << endl;
}

void executor::updateSet(update_t *exp)
{
    if (!catalogManager->isTableExists(exp->tableName))
    {
        cerr << "Table " + exp->tableName + " not exists." << endl << endl;
        return;
    }

    map<string, int> fieldRev;
    map<int, shared_ptr<value_t>> fieldToUpdate;
    table t = catalogManager->getTable(exp->tableName);
    for (int i = 0; i < t.getFieldLength(); i++)
        fieldRev[t.fields[i].name] = i;

    for (auto &vp : exp->fields)
    {
        if (fieldRev.count(vp.first))
        {
            int offset = fieldRev[vp.first];

            if (t.fields[offset].unique)
            {
                cerr << "Unique field " << vp.first << " cannot be changed." << endl << endl;
                return;
            }
            else if (vp.second->type == 'I' && t.fields[offset].type == 'F')
            {
                fieldToUpdate[offset].reset(new value_float(std::static_pointer_cast<value_int>(vp.second)->value));
            }
            else if (vp.second->type == t.fields[offset].type)
            {
                fieldToUpdate[offset] = vp.second;
            }
            else
            {
                cerr << "Field " << vp.first << " value type mismatch." << endl << endl;
                return;
            }
        }
        else
        {
            cerr << "Field " << vp.first << " not exists." << endl << endl;
            return;
        }
    }

    if (exp->where == nullptr)
    {
        exp->where.reset(new expression::from_result_t(true));
    }
    else
    {
        vector<shared_ptr<fieldref_t>> refs;
        exp->where->getFieldRef(refs);

        for (auto &ref : refs)
        {
            if (fieldRev.count(ref->tempName))
            {
                ref->fieldId = fieldRev[ref->tempName];
            }
            else
            {
                cerr << "Field " + ref->tempName + " not exists." << endl << endl;
                return;
            }
        }
    }

    auto count = recordingManager->update(exp->tableName, exp->where, fieldToUpdate);
    cerr << "Query finished, " << count << " rows affected." << endl << endl;
}
