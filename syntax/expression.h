#pragma once

#include <string>
#include "../record/memtable.h"
#include "basedef.h"
#include "../record/value.h"
#include <cstdint>
#include <algorithm>
using std::string;
using std::pair;

namespace expression
{
    class fieldref_t : public expression_t
    {
    public:
        int fieldId;
        string tempName;
        explicit fieldref_t(string tempName) : tempName(std::move(tempName)) { fieldId = -1; }
        fieldref_t(memtable_t &table, int fieldId) : tempName(table.fields[fieldId].name), fieldId(fieldId) {}
        virtual string to_string() const { return tempName; }
    };

    class evaluatable_t : public expression_t
    {
    public:
        virtual bool evaluate(tuple t) const = 0;
        virtual void getFieldRef(vector<shared_ptr<fieldref_t>> &refs) = 0;
        virtual pair<int,int> getPkeyRange(int pkfid) const { return { INT_MIN, INT_MAX }; }
    };

    enum OPERATOR
    {
        LESS_THAN,
        LESS_EQUAL,
        EQUAL,
        NOT_EQUAL,
        GREATER_THAN,
        GREATER_EQUAL,
        LOGIC_AND,
        LOGIC_OR,
    };

    class compare_t : public evaluatable_t
    {
    public:
        const shared_ptr<fieldref_t> left;
        const OPERATOR conjunction;
        const shared_ptr<value_t> right;

        compare_t(shared_ptr<fieldref_t> l, OPERATOR type, shared_ptr<value_t> r)
            : left(std::move(l)), conjunction(type), right(std::move(r)) {}

        bool evaluate(tuple t) const override
        {
            int cmp = t[left->fieldId]->compareTo(*right);
            if (conjunction == LESS_THAN) return cmp < 0;
            else if (conjunction == LESS_EQUAL) return cmp <= 0;
            else if (conjunction == EQUAL) return cmp == 0;
            else if (conjunction == NOT_EQUAL) return cmp != 0;
            else if (conjunction == GREATER_THAN) return cmp > 0;
            else if (conjunction == GREATER_EQUAL) return cmp >= 0;
            else assert(false || "ERROR CONJUNCTION"); return false;
        }

        string to_string() const override
        {
            const char *conj = nullptr;
            if (conjunction == LESS_THAN) conj = " < ";
            else if (conjunction == LESS_EQUAL) conj = " <= ";
            else if (conjunction == EQUAL) conj = " = ";
            else if (conjunction == NOT_EQUAL) conj = " != ";
            else if (conjunction == GREATER_THAN) conj = " > ";
            else if (conjunction == GREATER_EQUAL) conj = " >= ";
            return left->to_string() + conj + right->to_string();
        }

        void getFieldRef(vector<shared_ptr<fieldref_t>> &refs) override
        {
            refs.push_back(left);
        }

        pair<int,int> getPkeyRange(int pkfid) const override
        {
            // 不等于号、非数字、左侧非主键
            if (conjunction == NOT_EQUAL || right->type != 'I' || left->fieldId != pkfid)
                return std::make_pair(INT_MIN, INT_MAX);
            auto ptr = std::static_pointer_cast<value_int>(right);
            if (conjunction == EQUAL)
                return std::make_pair(ptr->value, ptr->value);
            else if (conjunction == GREATER_EQUAL || conjunction == GREATER_THAN)
                return std::make_pair(ptr->value, INT_MAX);
            else
                return std::make_pair(INT_MIN, ptr->value);
        }
    };

    class compare_t2 : public evaluatable_t
    {
    public:
        const shared_ptr<fieldref_t> left;
        const OPERATOR conjunction;
        const shared_ptr<fieldref_t> right;

        compare_t2(shared_ptr<fieldref_t> l, OPERATOR type, shared_ptr<fieldref_t> r)
                : left(std::move(l)), conjunction(type), right(std::move(r)) {}

        bool evaluate(tuple t) const override
        {
            int cmp = t[left->fieldId]->compareTo(*t[right->fieldId]);
            if (conjunction == LESS_THAN) return cmp < 0;
            else if (conjunction == LESS_EQUAL) return cmp <= 0;
            else if (conjunction == EQUAL) return cmp == 0;
            else if (conjunction == NOT_EQUAL) return cmp != 0;
            else if (conjunction == GREATER_THAN) return cmp > 0;
            else if (conjunction == GREATER_EQUAL) return cmp >= 0;
            else assert(false || "ERROR CONJUNCTION"); return false;
        }

        string to_string() const override
        {
            const char *conj = nullptr;
            if (conjunction == LESS_THAN) conj = " < ";
            else if (conjunction == LESS_EQUAL) conj = " <= ";
            else if (conjunction == EQUAL) conj = " = ";
            else if (conjunction == NOT_EQUAL) conj = " != ";
            else if (conjunction == GREATER_THAN) conj = " > ";
            else if (conjunction == GREATER_EQUAL) conj = " >= ";
            return left->to_string() + conj + right->to_string();
        }

        void getFieldRef(vector<shared_ptr<fieldref_t>> &refs) override
        {
            refs.push_back(left);
            refs.push_back(right);
        }
    };

    class connected_t : public evaluatable_t
    {
    public:
        const shared_ptr<evaluatable_t> left;
        const OPERATOR conjunction;
        const shared_ptr<evaluatable_t> right;

        connected_t(shared_ptr<evaluatable_t> l, OPERATOR type, shared_ptr<evaluatable_t>r)
            : left(std::move(l)), conjunction(type), right(std::move(r)) {}

        bool evaluate(tuple t) const override
        {
            if (conjunction == LOGIC_AND) return left->evaluate(t) && right->evaluate(t);
            else if (conjunction == LOGIC_OR) return left->evaluate(t) || right->evaluate(t);
            else assert(false || "ERROR CONJUNCTION"); return false;
        }

        string to_string() const override
        {
            const char *conj = nullptr;
            if (conjunction == LOGIC_AND) conj = ") AND (";
            else if (conjunction == LOGIC_OR) conj = ") OR (";
            return "(" + left->to_string() + conj + right->to_string() + ")";
        }

        void getFieldRef(vector<shared_ptr<fieldref_t>> &refs) override
        {
            left->getFieldRef(refs);
            right->getFieldRef(refs);
        }

        pair<int,int> getPkeyRange(int pkfid) const override
        {
            auto range1 = left->getPkeyRange(pkfid);
            auto range2 = right->getPkeyRange(pkfid);
            auto fail = std::make_pair(INT_MAX, INT_MIN);

            if (conjunction == LOGIC_AND)
            {
                if (range1 == fail || range2 == fail) return fail;
                if (range1.first > range2.second || range1.second < range2.first) return fail;
                return std::make_pair(std::max(range1.first, range2.first), std::min(range1.second, range2.second));
            }
            else
            {
                if (range1 == fail) return range2;
                if (range2 == fail) return range1;
                if (range1.first > range2.second || range1.second < range2.first)
                    return std::make_pair(INT_MIN, INT_MAX);
                return std::make_pair(std::min(range1.first, range2.first), std::max(range1.second, range2.second));
            }
        }
    };

    class from_result_t : public evaluatable_t
    {
    public:
        const bool res;
        from_result_t(bool o) : res(o) {}
        bool evaluate(tuple t) const override { return res; }
        string to_string() const override { return res ? "TRUE" : "FALSE"; }
        void getFieldRef(vector<shared_ptr<fieldref_t>> &refs) override {}
    }
    const const_yes(true), const_no(false);
};

typedef const shared_ptr<expression::evaluatable_t> &condition;
