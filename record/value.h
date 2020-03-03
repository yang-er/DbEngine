#pragma once

#include <string>
#include <cassert>
#include <memory>
#include <vector>
#include "../storage/block.h"
#include "../syntax/basedef.h"
using std::string;
using std::vector;
using std::shared_ptr;

class value_t : public expression::expression_t
{
public:
    const char type;
    explicit value_t(char type);
    virtual int size() const { return 4; }
    virtual void write(block blk, int offset) const = 0;
    virtual void read(block blk, int offset) = 0;
    virtual int compareTo(const value_t &b) const = 0;
    virtual shared_ptr<value_t> copy() const = 0;
};

class value_int : public value_t
{
public:
    int value;
    explicit value_int(int a);
    value_int();
    string to_string() const override;
    int compareTo(const value_t &b) const override;
    void write(block blk, int offset) const override;
    void read(block blk, int offset) override;
    shared_ptr<value_t> copy() const override;
};

class value_float : public value_t
{
public:
    float value;
    explicit value_float(float a);
    value_float();
    string to_string() const override;
    int compareTo(const value_t &b) const override;
    void write(block blk, int offset) const override;
    void read(block blk, int offset) override;
    shared_ptr<value_t> copy() const override;
};

class value_string : public value_t
{
public:
    string value;
    explicit value_string(string a);
    value_string();
    int size() const override { return block_t::STR_SIZE; }
    string to_string() const override;
    int compareTo(const value_t &b) const override;
    void write(block blk, int offset) const override;
    void read(block blk, int offset) override;
    shared_ptr<value_t> copy() const override;
};

typedef vector<shared_ptr<value_t>> tuple, Tuple;
