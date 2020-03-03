#pragma once

#include <string>
#include <map>
#include <memory>
#include <istream>
#include "basedef.h"
#include "expression.h"
using std::map;
using std::shared_ptr;

namespace tokenize
{
    using expression::expression_t;

    class token_t : public expression_t
    {
    public:
        const int tag;
        explicit token_t(int t) : tag(t) {}
        string to_string() const override { char op[2]; op[0] = tag, op[1] = 0; return string(op); }
    };

    typedef std::shared_ptr<token_t> token;

    class word_t : public token_t
    {
    public:
        const string cont;
        word_t(string s, int t) : token_t(t), cont(std::move(s)) {}
        string to_string() const override { return cont; }
        static map<string, token> registry;
    };

    enum tag_t
    {
        // 符号
        BRA = '(',
        KET = ')',
        COMMA = ',',
        SEMICOLON = ';',
        MATCHALL = '*',

        // 关键字
        CREATE = 201,
        DROP = 202,
        TABLE = 203,
        INDEX = 204,
        SELECT = 205,
        VERBOSE_SELECT = 283,
        INSERT = 206,
        DELETE = 207,
        QUIT = 208,
        EXECFILE = 209,
        SHOW = 200,
        FROM = 211,
        INTO = 212,
        WHERE = 213,
        OR = 271,
        ON = 214,
        AND = 215,
        UNIQUE = 216,
        PRIMARY = 217,
        KEY = 218,
        VALUES = 219,
        ORDER = 233,
        BY = 244,
        ASC = 245,
        DESC = 246,
        UPDATE = 281,
        SET = 282,
        TYPE = 257, //字段类型

        // 其他令牌
        STR = 222, //字符串
        INTNUM = 220, //整数
        FLOATNUM = 221, //浮点数
        OP = 222, //操作符
        ID = 264 //表名、索引名或字段名
    };

    class number_t : public token_t
    {
    public:
        const double value;
        explicit number_t(float t) : token_t(FLOATNUM), value(t) {}
        explicit number_t(int t) : token_t(INTNUM), value(t) {}
        string to_string() const override { return tag == INTNUM ? std::to_string(int(value)) : std::to_string(float(value)); }
    };

    class comparison_t : public word_t
    {
    public:
        const expression::OPERATOR op;
        comparison_t(string s, int tag, expression::OPERATOR op) : word_t(std::move(s), tag), op(op) {}
    };

    class lexer
    {
    private:
        char peek = ' ';
        bool end = false;

    public:
        std::istream& reader;
        const token scan(bool nl);
        bool isEnd() const { return end; }
        explicit lexer(std::istream& is);

        void readch()
        {
            int ch = reader.get();
            if (ch == -1) end = true, peek = -1;
            else peek = ch;
        }

        bool expected(char ch)
        {
            readch();
            if (peek != ch) return false;
            return peek = ' ', true;
        }
    };
};
