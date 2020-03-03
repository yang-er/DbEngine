#include "value.h"

value_t::value_t(char type) : type(type) {}

value_string::value_string() : value_t('S'), value("") {}
value_string::value_string(string a) : value_t('S'), value(std::move(a)) {}
string value_string::to_string() const { return value; }
void value_string::write(block blk, int offset) const { blk.writeString(offset, value); }
void value_string::read(block blk, int offset) { value = blk.readString(offset); }
shared_ptr<value_t> value_string::copy() const { return std::make_shared<value_string>(value); }

value_float::value_float(float a) : value_t('F'), value(a) {}
value_float::value_float() : value_float(0) {}
string value_float::to_string() const { return std::to_string(value); }
void value_float::write(block blk, int offset) const { blk.writeFloat(offset, value); }
void value_float::read(block blk, int offset) { value = blk.readFloat(offset); }
shared_ptr<value_t> value_float::copy() const { return std::make_shared<value_float>(value); }

value_int::value_int(int a) : value_t('I'), value(a) {}
value_int::value_int() : value_int(0) {}
string value_int::to_string() const { return std::to_string(value); }
void value_int::write(block blk, int offset) const { blk.writeInt(offset, value); }
void value_int::read(block blk, int offset) { value = blk.readInt(offset); }
shared_ptr<value_t> value_int::copy() const { return std::make_shared<value_int>(value); }

int value_string::compareTo(const value_t &b) const
{
    if (b.type != 'S') return false;
    return value != reinterpret_cast<const value_string &>(b).value;
}

template <typename T1, typename T2>
int __compareTo(T1 a, T2 b)
{
    return a < b ? -1 : a == b ? 0 : 1;
}

template <typename T1, typename T2>
int _compareTo(const T1 a, const value_t& b)
{
    return __compareTo(a, reinterpret_cast<const T2&>(b).value);
}

int value_float::compareTo(const value_t& b) const
{
    if (b.type == 'S') return false;
    return b.type == 'I' ? _compareTo<float, value_int>(value, b) : _compareTo<float, value_float>(value, b);
}

int value_int::compareTo(const value_t& b) const
{
    if (b.type == 'S') return false;
    return b.type == 'I' ? _compareTo<int, value_int>(value, b) : _compareTo<int, value_float>(value, b);
}
