#include <tuple>
#include <iostream>
#include <string>
#include <stdio.h>
#include "tuple_struct.h"
#include "sql/item.h"

using namespace std;

InMemoryTuple::InMemoryTuple(tuple<string, string, string, double> PublicContent){
  Content = PublicContent;
}

double InMemoryTuple::GetSelectivityForCondition(Item *condition){
    double selectivity = -1.0;
    std::size_t a = ItemToString(condition).find(get<0>(Content));
    std::size_t b = ItemToString(condition).find(get<2>(Content));
    if(a != std::string::npos && b != std::string::npos){
      selectivity = get<3>(Content);
    }
    return selectivity;
}