cmake_minimum_required(VERSION 3.14)
project(tinydb)

set(CMAKE_CXX_STANDARD 14)

add_executable(tinydb main.cpp storage/block.cpp storage/buffer.cpp catalog/field.cpp catalog/table.cpp catalog/index.cpp catalog/catalog.cpp record/recording.cpp indexing/bptree.cpp indexing/indexing.cpp record/memtable.cpp record/value.cpp syntax/token.cpp syntax/parser.cpp syntax/syntax_tree.cpp syntax/syntax_tree.h execution/executor.cpp execution/executor.h indexing/nodes_internal.cpp indexing/nodes_leaf.cpp)