#include <tuple>
#include <iostream>
#include <string>
#include <stdio.h>
#include "tuple_struct.h"

using namespace std;

InMemoryTuple::InMemoryTuple(tuple<string, string, string, double> Content);

InMemoryTuple::GetSelectivityForCondition(Item *condition){
    double selectivity = -1.0;
    std::size_t a = ItemToString(condition).find(Content[0]);
    std::size_t b = ItemToString(condition).find(Content[2]);
    if(a != std::string::npos && b != std::string::npos){
      selectivity = Content[3];
    }
    return selectivity;
}