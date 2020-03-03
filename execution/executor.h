#pragma once

#include "../syntax/basedef.h"
#include "../catalog/table.h"
#include "../syntax/syntax_tree.h"
using expression::create_index_t;
using expression::create_table_t;
using expression::quit_t;
using expression::execfile_t;
using expression::drop_table_t;
using expression::drop_index_t;
using expression::insert_into_t;
using expression::delete_t;
using expression::select_t;
using expression::show_catalog_t;
using expression::update_t;

class executor
{
private:
    bool ended = false;

public:
    bool isEnded() const;
    void createTable(create_table_t *exp);
    void createIndex(create_index_t *exp);
    void quit(quit_t *exp);
    void execfile(execfile_t *exp);
    void dropTable(drop_table_t *exp);
    void dropIndex(drop_index_t *exp);
    void insertInto(insert_into_t *exp);
    void deleteFrom(delete_t *exp);
    void selectFrom(select_t *exp);
    void showCatalog(show_catalog_t *exp);
    void updateSet(update_t *exp);
};
