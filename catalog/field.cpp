#define _CRT_SECURE_NO_WARNINGS
#include "field.h"
#include <cassert>
#include <cstring>

field_t::field_t()
{
    type = '?';
    length = 0;
    offset = -1;
    unique = false;
}

field_t::field_t(string name, char type, bool unique)
    : name(std::move(name)), type(type), unique(unique), length(type=='S'?256:4)
{
    // 几个限制
    //assert(name.length() < 28);
    assert(type == 'S' || type == 'I' || type == 'F');
    offset = 0;
}

field_t field_t::deserialize(FILE *fp)
{
    // 内存排布：类型1|UniqueFlag1|字段名称30
    char buf[32];
    memset(buf, 0, sizeof(buf));
    fread(buf, 32, 1, fp);
    assert(buf[0] == '@' && buf[1] == 'F');
    return field_t(buf+4, buf[2], buf[3] == 'U');
}

void field_t::serialize(FILE *fp)
{
    assert(name.length() < 28);
    char buf[32];
    memset(buf, 0, sizeof(buf));
    buf[0] = '@';
    buf[1] = 'F';
    buf[2] = type;
    buf[3] = unique ? 'U' : 'N';
    sprintf(buf+4, "%s", name.data());
    fwrite(buf, 32, 1, fp);
}
