#pragma once

#include <string>
using std::string;

namespace expression
{
    class expression_t
    {
    public:
        virtual string to_string() const = 0;
    };
}