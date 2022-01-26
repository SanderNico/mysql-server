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

    for(std::vector< tuple<string, string, string, double>>::size_type it = 0; it != Content.size(); it++){
      std::size_t a = ItemToString(condition).find(get<0>(Content[it]));
      std::size_t b = ItemToString(condition).find(get<2>(Content[it]));
      if(a != std::string::npos && b != std::string::npos){
        selectivity = get<3>(Content[it]);
      }
      return selectivity;
    }
    return selectivity;
}