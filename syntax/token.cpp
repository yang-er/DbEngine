#include "token.h"
#include <cctype>
#include <sstream>
#include <cmath>
using std::tolower;
using std::make_shared;

#define REG_OP(a, b) word_t::registry[a] = make_shared<comparison_t>(a, OP, b)
#define REG_TYPE(a) word_t::registry[a] = make_shared<word_t>(a, TYPE)
#define REG_WORD(a) word_t::registry[#a] = make_shared<word_t>(#a, a)

namespace tokenize
{
    map<string, token> word_t::registry;

    lexer::lexer(std::istream &is) : reader(is)
    {
        if (word_t::registry.empty())
        {
            REG_OP("<", expression::LESS_THAN);
            REG_OP(">", expression::GREATER_THAN);
            REG_OP("=", expression::EQUAL);
            REG_OP("<>", expression::NOT_EQUAL);
            REG_OP("<=", expression::LESS_EQUAL);
            REG_OP(">=", expression::GREATER_EQUAL);

            REG_TYPE("INT");
            REG_TYPE("FLOAT");
            REG_TYPE("STRING");

            REG_WORD(CREATE);
            REG_WORD(DROP);
            REG_WORD(TABLE);
            REG_WORD(INDEX);
            REG_WORD(SELECT);
            REG_WORD(VERBOSE_SELECT);
            REG_WORD(INSERT);
            REG_WORD(DELETE);
            REG_WORD(QUIT);
            REG_WORD(EXECFILE);
            REG_WORD(SHOW);
            REG_WORD(FROM);
            REG_WORD(INTO);
            REG_WORD(WHERE);
            REG_WORD(ON);
            REG_WORD(AND);
            REG_WORD(OR);
            REG_WORD(UNIQUE);
            REG_WORD(PRIMARY);
            REG_WORD(KEY);
            REG_WORD(VALUES);
            REG_WORD(ORDER);
            REG_WORD(BY);
            REG_WORD(ASC);
            REG_WORD(DESC);
            REG_WORD(UPDATE);
            REG_WORD(SET);
        }
    }

    shared_ptr<token_t> TOKEOF = make_shared<token_t>(-1);

    const token lexer::scan(bool nl)
    {
        for (; ; readch())
            if (peek == ' ' || peek == '\t' || peek == '\r')
                continue;
            else if (peek == '\n')
                nl && printf("> ");
            else
                break;
        if (end) return TOKEOF;

        /* 下面开始分割关键字，标识符等信息 */
        switch (peek)
        {
            case '=':
                return peek = ' ', word_t::registry["="];
            case '>':
                if (expected('='))
                    return word_t::registry[">="];
                else
                    return word_t::registry[">"];
            case '<':
                if (expected('='))
                    return word_t::registry["<="];
                else if (peek == '>')
                    return peek = ' ', word_t::registry["<>"];
                else
                    return word_t::registry["<"];
            case '!':
                if (expected('='))
                    return word_t::registry["<>"];
                else
                    return make_shared<token_t>('!');
            default:
                break;
        }


        /* 下面是对数字的识别，根据文法的规定的话，这里的
         * 数字能够识别整数和小数.
         */
        if (isdigit(peek))
        {
            double value = 0;

            do
            {
                value = 10 * value + (peek - '0');
                readch();
            }
            while (isdigit(peek));

            if (peek == '.')
            {
                readch();
                int i = 1;

                do
                {
                    value = value + (peek - '0') * pow(0.1, i++);
                    readch();
                }
                while (isdigit(peek));

                return make_shared<number_t>((float)value);
            }
            else
            {
                return make_shared<number_t>((int)value);
            }
        }


        /*
         * 对字符串进行识别
         */
        if (peek == '\'')
        {
            std::stringstream sb;
            sb << "";
            readch();

            while (peek != '\'' && peek != ';')
            {
                sb << peek;
                readch();
            }

            token w;
            if (peek == ';')
                w = make_shared<token_t>(peek);
            else
                w = make_shared<word_t>(sb.str(), STR);
            readch();
            return w;
        }


        /*
         * 关键字或者是标识符的识别
         */
        if (isalpha(peek))
        {
            std::stringstream sb;

            /* 首先得到整个的一个分割 */
            do
            {
                sb << peek;
                readch();
            }
            while (isalpha(peek) || isdigit(peek) || peek == '_' || peek == '.' || peek == '&');

            /* 判断是关键字还是标识符 */
            string s = sb.str();
            return word_t::registry.count(s) > 0 ? word_t::registry[s] : make_shared<word_t>(s, ID);
        }

        /* peek中的任意字符都被认为是词法单元返回 */
        char tmp = peek; peek = ' ';
        return make_shared<token_t>(tmp);
    }
};
