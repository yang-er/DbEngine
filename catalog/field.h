#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
using std::string;

class field_t
{
public:
    string name;
    char type;
    int length, offset;
    bool unique;

    field_t();
    field_t(string name, char type, bool unique);

    static field_t deserialize(FILE *fp);
    void serialize(FILE *fp);
};

typedef field_t &field;
