#include <tuple>
#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include "tuple_struct.h"
#include "sql/item.h"

using namespace std;

InMemoryTuple::InMemoryTuple(vector <tuple<string, string, string, double> > PublicContent){
  Content = PublicContent;
}

double InMemoryTuple::GetSelectivityForCondition(Item *condition){
    double selectivity = -1.0;

    for(auto it = begin (Content); it != end (Content); it++){
      std::size_t a = ItemToString(condition).find(it->get<0>());
      std::size_t b = ItemToString(condition).find(it->get<2>());
      if(a != std::string::npos && b != std::string::npos){
        selectivity = it->get<3>();
      }
      return selectivity;
    }
    return selectivity;
}