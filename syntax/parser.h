#pragma once

#include "basedef.h"
#include <vector>
#include <functional>
#include "token.h"
#include "../execution/executor.h"
using std::vector;
using std::pair;
using std::function;
using expression::expression_t;
using std::shared_ptr;
using tokenize::lexer;
using tokenize::token_t;

using state_stack_t = vector<shared_ptr<expression_t>>;
using transact_edge_t = function<bool(state_stack_t&, shared_ptr<token_t>)>;
using token_tag_t = tokenize::tag_t;
using next_state_t = pair<pair<token_tag_t, int>, transact_edge_t>;

class parser
{
public:
    lexer& tokens;
    explicit parser(lexer& lexer);
    static vector<vector<next_state_t>> states;
    static void init();
    void process(executor *, bool continueOnError);
};
