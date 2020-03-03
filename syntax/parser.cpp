#include "parser.h"
#include "syntax_tree.h"
#include <iostream>
#include <queue>
using std::cerr;
using std::queue;
using std::endl;
using std::make_pair;
using tokenize::word_t;
using tokenize::number_t;
using tokenize::comparison_t;
using tokenize::tag_t;
using namespace std::placeholders;
using namespace expression;

#define LINK_STATE(OOO,AAA,BBB,CCC) states[OOO].emplace_back(make_pair(tokenize::AAA, BBB), CCC)

vector<vector<next_state_t>> parser::states;

parser::parser(lexer &lexer) : tokens(lexer)
{
    if (states.empty()) init();
}

template <typename T>
bool do_add_stack(state_stack_t& stack1, shared_ptr<token_t>)
{
    if (!stack1.empty()) return false;
    stack1.emplace_back(new T);
    return true;
}

template <typename T>
bool do_push(state_stack_t& stack1, shared_ptr<token_t> s)
{
    stack1.emplace_back(s);
    return true;
}

shared_ptr<evaluatable_t> parseExpression(const shared_ptr<token_t> &fr, queue<shared_ptr<token_t>> &q)
{
    if (fr->tag != tokenize::ID) return nullptr;
    auto lhs = std::make_shared<fieldref_t>(fr->to_string());

    if (q.empty() || q.front()->tag != tokenize::OP) return nullptr;
    auto op = std::static_pointer_cast<comparison_t>(q.front())->op;
    q.pop();

    if (q.empty()) return nullptr;
    auto res = q.front(); q.pop();
    if (res->tag == tokenize::ID)
    {
        auto rhs = std::make_shared<fieldref_t>(res->to_string());
        return std::make_shared<compare_t2>(lhs, op, rhs);
    }
    else if (res->tag == tokenize::INTNUM || res->tag == tokenize::FLOATNUM)
    {
        auto tmp = std::static_pointer_cast<number_t>(res);
        shared_ptr<value_t> rhs(res->tag == tokenize::INTNUM
                ? (value_t*)new value_int(tmp->value)
                : new value_float(tmp->value));
        return std::make_shared<compare_t>(lhs, op, rhs);
    }
    else if (res->tag == tokenize::STR && (op == EQUAL || op == NOT_EQUAL))
    {
        auto tmp = std::static_pointer_cast<word_t>(res);
        return std::make_shared<compare_t>(lhs, op, std::make_shared<value_string>(tmp->cont));
    }
    else
    {
        return nullptr;
    }
}

shared_ptr<evaluatable_t> parseCondition(queue<shared_ptr<token_t>> &q, tokenize::tag_t end)
{
    shared_ptr<evaluatable_t> tmpCondRoot, tmpExp;
    auto tok = q.front(); q.pop();

    if (tok->tag == tag_t::BRA)
        tmpCondRoot = parseCondition(q, tag_t::KET);
    else if (tok->tag == tag_t::ID)
        tmpCondRoot = parseExpression(tok, q);
    else
        return nullptr;
    if (tmpCondRoot == nullptr) return nullptr;

    tok = q.front(); q.pop();
    while (tok->tag != end)
    {
        if (tok->tag == tag_t::AND || tok->tag == tag_t::OR)
        {
            auto op = tok->tag == tag_t::AND ? LOGIC_AND : LOGIC_OR;
            tok = q.front(); q.pop();
            if (tok->tag == tag_t::BRA)
                tmpExp = parseCondition(q, tag_t::KET);
            else if (tok->tag == tag_t::ID)
                tmpExp = parseExpression(tok, q);
            else
                tmpExp = nullptr;

            if (tmpExp == nullptr)
                return nullptr;
            tmpCondRoot = std::make_shared<connected_t>(tmpCondRoot, op, tmpExp);
        }
        else if (tok->tag == end)
        {

        }
        else
        {
            return nullptr;
        }

        tok = q.front(); q.pop();
    }

    return tmpCondRoot;
}

void parser::init()
{
    states.resize(60);
    auto do_begins = [] (state_stack_t& stack1, shared_ptr<token_t>) { return stack1.empty(); };
    auto do_nothing = [] (state_stack_t&, shared_ptr<token_t>) { return true; };

    LINK_STATE(0, SEMICOLON, -1, [] (state_stack_t& stack1, shared_ptr<token_t>) { return stack1.size() == 1; });

    // CREATE TABLE / INDEX
    LINK_STATE(0, CREATE, 1, do_begins);
    LINK_STATE(1, TABLE, 9, do_add_stack<create_table_t>);// CREATE TABLE

    LINK_STATE(9, ID, 10, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto ctt = (create_table_t*)stack1.front().get();
        ctt->tableName = t->to_string();
        return true;
    });

    LINK_STATE(10, BRA, 11, do_nothing); // CREATE TABLE tn (
    LINK_STATE(11, PRIMARY, 12, do_nothing); // CT( PRIMARY
    LINK_STATE(12, KEY, 13, do_nothing); // CT( PRIMARY KEY
    LINK_STATE(13, BRA, 14, do_nothing); // CT( PRIMARY KEY (

    LINK_STATE(14, ID, 15, [] (state_stack_t& stack2, shared_ptr<token_t> t) // CT(PK(xx
    {
        auto ctt = (create_table_t*)stack2.front().get();
        int target = -1;
        for (int i = 0; i < ctt->fields.size(); i++)
            if (ctt->fields[i].name == t->to_string())
                target = i;
        if (target == -1) return false;
        ctt->primaryKey = target; return true;
    });

    LINK_STATE(15, KET, 16, do_nothing); // CT(PK(xx)
    LINK_STATE(16, KET, 0, do_nothing); // CT(PK(xx))

    LINK_STATE(11, ID, 17, [] (state_stack_t& stk, shared_ptr<token_t> t) // CT( FieldName
    {
        auto ctt = (create_table_t*)stk.front().get();
        for (auto &f : ctt->fields)
            if (f.name == t->to_string())
                return false;
        stk.push_back(t);
        return true;
    });

    LINK_STATE(17, TYPE, 18, [] (state_stack_t& stk, shared_ptr<token_t> t) // CT( FieldName FieldType
    {
        stk.push_back(t);
        return true;
    });

    auto do_add_field = [] (state_stack_t& stk, shared_ptr<token_t>, bool unique)
    {
        word_t type = *((word_t*)stk.back().get());
        stk.pop_back();
        word_t name = *((word_t*)stk.back().get());
        stk.pop_back();
        auto ctt = (create_table_t*)stk.front().get();
        ctt->fields.emplace_back(name.cont, type.cont[0], unique);
        return true;
    };

    LINK_STATE(18, COMMA, 11, std::bind(do_add_field, _1, _2, false)); // CT( FieldName FieldType,
    LINK_STATE(18, UNIQUE, 19, do_nothing); // CT( FieldName FieldType,
    LINK_STATE(19, COMMA, 11, std::bind(do_add_field, _1, _2, true)); // CT( FieldName FieldType UNIQUE,

    LINK_STATE(1, INDEX, 20, do_add_stack<create_index_t>); // CREATE INDEX

    LINK_STATE(20, ID, 21, [] (state_stack_t& stack1, shared_ptr<token_t> b) // CREATE INDEX idx
    {
        auto cit = (create_index_t*)stack1.front().get();
        cit->indexName = b->to_string();
        return true;
    });

    LINK_STATE(21, ON, 22, do_nothing); // CREATE INDEX idx ON

    LINK_STATE(22, ID, 23, [] (state_stack_t& stack1, shared_ptr<token_t> b) // CREATE INDEX idx ON tbl
    {
        auto cit = (create_index_t*)stack1.front().get();
        cit->tableName = b->to_string();
        return true;
    });

    LINK_STATE(23, BRA, 24, do_nothing);  // CREATE INDEX idx ON tbl(

    LINK_STATE(24, ID, 25, [] (state_stack_t& stack1, shared_ptr<token_t> b) // CREATE INDEX idx ON tbl(xx
    {
        auto cit = (create_index_t*)stack1.front().get();
        cit->fieldName = b->to_string();
        return true;
    });

    LINK_STATE(25, KET, 0, do_nothing); // CREATE INDEX idx ON tbl(xx)

    // QUIT
    LINK_STATE(0, QUIT, 0, do_add_stack<quit_t>);

    // DROP
    LINK_STATE(0, DROP, 2, do_begins);

    LINK_STATE(2, TABLE, 26, do_add_stack<drop_table_t>); // DROP TABLE

    LINK_STATE(26, ID, 0, [] (state_stack_t& stack1, shared_ptr<token_t> b) // DROP TABLE tableName
    {
        auto cit = (drop_table_t*)stack1.front().get();
        cit->tableName = b->to_string();
        return true;
    });

    LINK_STATE(2, INDEX, 27, do_add_stack<drop_index_t> ); // DROP INDEX

    LINK_STATE(27, ID, 0, [] (state_stack_t& stack1, shared_ptr<token_t> b) // DROP INDEX indexName
    {
        auto cit = (drop_index_t*)stack1.front().get();
        cit->indexName = b->to_string();
        return true;
    });

    // EXECFILE
    LINK_STATE(0, EXECFILE, 3, do_add_stack<execfile_t>);

    LINK_STATE(3, ID, 0, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (execfile_t*)stack1.front().get();
        eft->fileName = t->to_string();
        return true;
    });

    // INSERT
    LINK_STATE(0, INSERT, 4, do_begins);
    LINK_STATE(4, INTO, 30, do_add_stack<insert_into_t>);

    LINK_STATE(30, ID, 31, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (insert_into_t*)stack1.front().get();
        eft->tableName = t->to_string();
        return true;
    });

    LINK_STATE(31, VALUES, 32, do_nothing);
    LINK_STATE(32, BRA, 33, do_nothing);

    LINK_STATE(33, STR, 34, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (insert_into_t*)stack1.front().get();
        eft->tuple.emplace_back(new value_string(t->to_string()));
        return true;
    });

    LINK_STATE(33, INTNUM, 34, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (insert_into_t*)stack1.front().get();
        auto wrd = std::static_pointer_cast<number_t>(t);
        eft->tuple.emplace_back(new value_int(wrd->value));
        return true;
    });

    LINK_STATE(33, FLOATNUM, 34, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (insert_into_t*)stack1.front().get();
        auto wrd = std::static_pointer_cast<number_t>(t);
        eft->tuple.emplace_back(new value_float(wrd->value));
        return true;
    });

    LINK_STATE(34, COMMA, 33, do_nothing);
    LINK_STATE(34, KET, 0, do_nothing);

    // SHOW TABLES / INDEXES
    LINK_STATE(0, SHOW, 5, do_begins);

    LINK_STATE(5, ID, 0, ([] (state_stack_t& stack1, shared_ptr<token_t> b)
    {
        bool tbl, idx;
        string ot = b->to_string();
        if (ot == "TABLES")
            tbl = true, idx = false;
        else if (ot == "INDEXES")
            tbl = false, idx = true;
        else if (ot == "CATALOG")
            tbl = true, idx = true;
        else
            return false;
        stack1.emplace_back(new show_catalog_t(tbl, idx));
        return true;
    }));


    // DELETE FROM xx WHERE ?
    LINK_STATE(0, DELETE, 6, do_begins);
    LINK_STATE(6, FROM, 36, do_add_stack<delete_t>);

    LINK_STATE(36, ID, 37, [] (state_stack_t& stack1, shared_ptr<token_t> b)
    {
        auto set = (delete_t*)stack1.front().get();
        set->tableName = b->to_string();
        return true;
    });

    LINK_STATE(37, SEMICOLON, -1, do_nothing);
    LINK_STATE(37, WHERE, 40, do_nothing);


    // WHERE STATES
    LINK_STATE(40, BRA, 40, do_push<token_t>);
    LINK_STATE(40, KET, 40, do_push<token_t>);
    LINK_STATE(40, COMMA, 40, do_push<token_t>);
    LINK_STATE(40, STR, 40, do_push<word_t>);
    LINK_STATE(40, INTNUM, 40, do_push<number_t>);
    LINK_STATE(40, FLOATNUM, 40, do_push<number_t>);
    LINK_STATE(40, OP, 40, do_push<comparison_t>);
    LINK_STATE(40, OR, 40, do_push<word_t>);
    LINK_STATE(40, AND, 40, do_push<word_t>);
    LINK_STATE(40, ID, 40, do_push<word_t>);

    // ORDER BY
    auto do_write_where = [] (state_stack_t& stack1, shared_ptr<token_t>, bool u)
    {
        queue<shared_ptr<token_t>> q;
        for (int i = 1; i < stack1.size(); i++)
            q.push(std::static_pointer_cast<token_t>(stack1[i]));
        q.push(std::make_shared<token_t>(';'));
        auto cond = parseCondition(q, tag_t::SEMICOLON);
        if (cond == nullptr) return false;
        auto set = (selectable_executable_t*)stack1.front().get();
        if (u && !set->acceptOrderBy) return false;
        set->where = cond;
        stack1.erase(stack1.begin()+1, stack1.end());
        return true;
    };

    LINK_STATE(40, SEMICOLON, -1, std::bind(do_write_where, _1, _2, false));
    LINK_STATE(40, ORDER, 41, std::bind(do_write_where, _1, _2, true));
    LINK_STATE(41, BY, 42, do_nothing);

    LINK_STATE(42, ID, 43, [] (state_stack_t& stack1, shared_ptr<token_t> b)
    {
        auto set = (select_t*)stack1.front().get();
        set->orderBy = b->to_string();
        set->orderByAsc = true;
        return true;
    });

    LINK_STATE(43, ASC, 0, do_nothing);

    LINK_STATE(43, DESC, 0, [] (state_stack_t& stack1, shared_ptr<token_t>)
    {
        auto set = (select_t*)stack1.front().get();
        set->orderByAsc = false;
        return true;
    });

    LINK_STATE(43, SEMICOLON, -1, do_nothing);


    // SELECT
    LINK_STATE(0, VERBOSE_SELECT, 8, [](state_stack_t& stack1, shared_ptr<token_t>)
    {
        stack1.emplace_back(new select_t);
        auto set = (select_t*)stack1.front().get();
        set->verbose = true;
        return true;
    });

    LINK_STATE(0, SELECT, 8, do_add_stack<select_t>);
    LINK_STATE(8, MATCHALL, 44, do_nothing);

    auto do_add_projection = [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto set = (select_t*)stack1.front().get();
        set->projection.push_back(t->to_string());
        return true;
    };

    LINK_STATE(8, ID, 45, do_add_projection);
    LINK_STATE(45, COMMA, 46, do_nothing);
    LINK_STATE(46, ID, 45, do_add_projection);
    LINK_STATE(45, FROM, 47, do_nothing);
    LINK_STATE(44, FROM, 47, do_nothing);

    LINK_STATE(47, ID, 48, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto set = (select_t*)stack1.front().get();
        set->tableNames.push_back(t->to_string());
        return true;
    });

    LINK_STATE(48, COMMA, 47, do_nothing);
    LINK_STATE(48, SEMICOLON, -1, do_nothing);
    LINK_STATE(48, WHERE, 40, do_nothing);
    LINK_STATE(48, ORDER, 41, do_nothing);

    // UPDATE
    LINK_STATE(0, UPDATE, 7, do_add_stack<update_t>);

    LINK_STATE(7, ID, 50, [] (state_stack_t& stack1, shared_ptr<token_t> b)
    {
        auto set = (update_t*)stack1.front().get();
        set->tableName = b->to_string();
        return true;
    });

    LINK_STATE(50, SET, 51, do_nothing);

    LINK_STATE(51, ID, 52, [] (state_stack_t& stack1, shared_ptr<token_t> b)
    {
        auto set = (update_t*)stack1.front().get();
        set->fields.emplace_back(b->to_string(), nullptr);
        return true;
    });

    LINK_STATE(52, OP, 53, [] (state_stack_t& stack1, shared_ptr<token_t> b)
    {
        auto b2 = std::static_pointer_cast<comparison_t>(b);
        return b2->op == OPERATOR::EQUAL;
    });

    LINK_STATE(53, STR, 54, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (update_t*)stack1.front().get();
        eft->fields.back().second.reset(new value_string(t->to_string()));
        return true;
    });

    LINK_STATE(53, INTNUM, 54, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (update_t*)stack1.front().get();
        auto wrd = std::static_pointer_cast<number_t>(t);
        eft->fields.back().second.reset(new value_int(wrd->value));
        return true;
    });

    LINK_STATE(53, FLOATNUM, 54, [] (state_stack_t& stack1, shared_ptr<token_t> t)
    {
        auto eft = (update_t*)stack1.front().get();
        auto wrd = std::static_pointer_cast<number_t>(t);
        eft->fields.back().second.reset(new value_float(wrd->value));
        return true;
    });

    LINK_STATE(54, COMMA, 51, do_nothing);
    LINK_STATE(54, SEMICOLON, -1, do_nothing);
    LINK_STATE(54, WHERE, 40, do_nothing);
}

void parser::process(executor *executor, bool continueOnError)
{
    int state = 0;
    state_stack_t stk;
    shared_ptr<token_t> space(new token_t(' ')), last = space;

    while (!tokens.isEnd() && !executor->isEnded())
    {
        auto token = tokens.scan(continueOnError);
        if (token->tag == -1) break;
        bool found_solver = false;
        for (auto &nxt : states[state])
        {
            if (nxt.first.first == token->tag)
            {
                found_solver = true;
                if (nxt.second(stk, token))
                    state = nxt.first.second;
                else
                    found_solver = false;
                break;
            }
        }

        if (state == 3 && !continueOnError)
            found_solver = false;

        if (!found_solver)
        {
            if (token->tag != ';')
            {
                while (true)
                {
                    auto t2k = tokens.scan(continueOnError);
                    if (t2k->tag == ';') break;
                }
            }

            cerr << "Syntax error near "
                 << last->to_string()
                 << " "
                 << token->to_string()
                 << "."
                 << endl << endl;

            token = space;
            stk.clear();
            state = 0;
            if (!continueOnError) break;
        }

        if (state == -1)
        {
            assert(stk.size() == 1);
            auto exec = std::static_pointer_cast<executable_t>(stk.front());
            exec->execute(executor);
            state = 0;
            last = space;
            stk.clear();
        }
        else
        {
            last = token;
        }
    }
}
