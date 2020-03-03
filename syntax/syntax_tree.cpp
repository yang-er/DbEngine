#define _CRT_SECURE_NO_WARNINGS
#include "syntax_tree.h"
#include "../execution/executor.h"

namespace expression
{
    void create_table_t::execute(executor *e)
    {
        e->createTable(this);
    }

    string create_table_t::to_string() const
    {
        char buf[4096]; buf[0] = 0; int i = 0;
        i += sprintf(buf+i, "CREATE TABLE %s\n(\n", tableName.data());
        for (auto &j : fields)
            i += sprintf(buf+i, "  %s %s%s,\n",
                 j.name.data(),
                 j.type == 'I' ? "INT" : j.type == 'F' ? "FLOAT" : "STRING",
                 j.unique ? " UNIQUE" : "");
        if (primaryKey != -1)
            i += sprintf(buf+i, "  PRIMARY KEY (%s)\n", fields[primaryKey].name.data());
        i += sprintf(buf+i, ");");
        return buf;
    }

    string create_index_t::to_string() const
    {
        char buf[4096]; buf[0] = 0;
        sprintf(buf, "CREATE INDEX %s ON %s(%s);",
                indexName.data(),
                tableName.data(),
                fieldName.data());
        return buf;
    }

    void create_index_t::execute(executor *e)
    {
        e->createIndex(this);
    }

    void quit_t::execute(executor *e)
    {
        e->quit(this);
    }

    string quit_t::to_string() const
    {
        return "QUIT;";
    }

    string execfile_t::to_string() const
    {
        return "EXECFILE " + fileName + ";";
    }

    void execfile_t::execute(executor *e)
    {
        e->execfile(this);
    }

    void drop_table_t::execute(executor *e)
    {
        e->dropTable(this);
    }

    string drop_table_t::to_string() const
    {
        return "DROP TABLE " + tableName + ";";
    }

    void drop_index_t::execute(executor *e)
    {
        e->dropIndex(this);
    }

    string drop_index_t::to_string() const
    {
        return "DROP INDEX " + indexName + ";";
    }

    string insert_into_t::to_string() const
    {
        char buf[4096]; buf[0] = 0; int i = 0;
        i += sprintf(buf+i, "INSERT INTO %s VALUES (", tableName.data());
        i += sprintf(buf+i, "%s", tuple.front()->to_string().data());
        for (int j = 1; j < tuple.size(); j++)
            i += sprintf(buf+i, ", %s", tuple[j]->to_string().data());
        i += sprintf(buf+i, ");");
        return buf;
    }

    void insert_into_t::execute(executor *e)
    {
        e->insertInto(this);
    }

    show_catalog_t::show_catalog_t(bool t, bool i)
    {
        showIndex = i;
        showTable = t;
    }

    void show_catalog_t::execute(executor *e)
    {
        e->showCatalog(this);
    }

    string show_catalog_t::to_string() const
    {
        if (!showTable)
            return "SHOW INDEXES;";
        else if (!showIndex)
            return "SHOW TABLES;";
        return "SHOW CATALOG;";
    }

    selectable_executable_t::selectable_executable_t(bool acOb) : acceptOrderBy(acOb)
    {
    }

    select_t::select_t() : selectable_executable_t(true), orderByAsc(true), verbose(false) {}

    delete_t::delete_t() : selectable_executable_t(false) {}

    string select_t::to_string() const
    {
        char buf[4096]; buf[0] = 0; int i = 0;
        i += sprintf(buf+i, "SELECT ");
        for (int j = 0; j < projection.size(); j++)
            i += sprintf(buf+i, "%s%c ", projection[j].data(), ", "[j==projection.size()-1]);
        if (projection.empty())
            i += sprintf(buf+i, "*  ");
        i += sprintf(buf+i, "FROM ");
        for (int j = 0; j < tableNames.size(); j++)
            i += sprintf(buf+i, "%s%c ", tableNames[j].data(), ", "[j==tableNames.size()-1]);
        if (where != nullptr)
            i += sprintf(buf+i, "WHERE %s ", where->to_string().data());
        if (!orderBy.empty())
            i += sprintf(buf+i, "ORDER BY %s %s ", orderBy.data(), orderByAsc ? "ASC" : "DESC");
        buf[i-1] = ';';
        return buf;
    }

    void select_t::execute(executor *e)
    {
        e->selectFrom(this);
    }

    string delete_t::to_string() const
    {
        auto o = "DELETE FROM " + tableName;
        if (where != nullptr)
            o += " WHERE " + where->to_string();
        return o + ";";
    }

    void delete_t::execute(executor *e)
    {
        e->deleteFrom(this);
    }

    update_t::update_t() : selectable_executable_t(false) {}

    string update_t::to_string() const
    {
        char buf[4096]; buf[0] = 0; int i = 0;
        i += sprintf(buf+i, "UPDATE %s SE", tableName.data());
        for (int j = 0; j < fields.size(); j++)
            i += sprintf(buf+i, "%c %s = %s", ",T"[j==0], fields[j].first.data(), fields[j].second->to_string().data());
        if (where != nullptr)
            i += sprintf(buf+i, " WHERE %s", where->to_string().data());
        buf[i] = ';'; buf[i+1] = 0;
        return buf;
    }

    void update_t::execute(executor *e)
    {
        e->updateSet(this);
    }
};
