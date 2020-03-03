#pragma once

#include "basedef.h"
#include "../catalog/table.h"
#include "../record/value.h"
#include "expression.h"
#include <memory>
using std::shared_ptr;

class executor;

namespace expression
{
    class executable_t : public expression_t
    {
    public:
        virtual void execute(executor *) = 0;
    };

    class create_table_t : public executable_t
    {
    public:
        string tableName;
        int primaryKey = -1;
        vector<field_t> fields;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class create_index_t : public executable_t
    {
    public:
        string tableName;
        string fieldName;
        string indexName;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class execfile_t : public executable_t
    {
    public:
        string fileName;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class quit_t : public executable_t
    {
    public:
        string fileName;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class drop_table_t : public executable_t
    {
    public:
        string tableName;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class drop_index_t : public executable_t
    {
    public:
        string indexName;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class insert_into_t : public executable_t
    {
    public:
        string tableName;
        vector<shared_ptr<value_t>> tuple;
        void execute(executor *e) override;
        string to_string() const override;
    };

    class show_catalog_t : public executable_t
    {
    public:
        bool showTable;
        bool showIndex;
        show_catalog_t(bool t, bool i);
        void execute(executor *e) override;
        string to_string() const override;
    };

    class selectable_executable_t : public executable_t
    {
    public:
        const bool acceptOrderBy;
        shared_ptr<evaluatable_t> where;
        explicit selectable_executable_t(bool acOb);
    };

    class select_t : public selectable_executable_t
    {
    public:
        vector<string> projection;
        string orderBy;
        bool orderByAsc;
        bool verbose;
        vector<string> tableNames;
        select_t();
        void execute(executor *e) override;
        string to_string() const override;
    };

    class delete_t : public selectable_executable_t
    {
    public:
        string tableName;
        delete_t();
        void execute(executor *e) override;
        string to_string() const override;
    };

    class update_t : public selectable_executable_t
    {
    public:
        string tableName;
        vector<pair<string, shared_ptr<value_t>>> fields;
        update_t();
        void execute(executor *e) override;
        string to_string() const override;
    };
};
