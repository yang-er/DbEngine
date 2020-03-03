#pragma once

#include "../catalog/field.h"
#include "value.h"
#include <vector>
#include <memory>
using std::vector;
using std::shared_ptr;

class memtable_t
{
public:
    string tmpName;
    vector<field_t> fields;
    vector<Tuple> tuples;
};
