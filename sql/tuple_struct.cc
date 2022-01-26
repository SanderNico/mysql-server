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

    for(vector <tuple<string, string, string, double>>::const_iterator i = Content.begin(); i != Content.end(); i++){
      std::size_t a = ItemToString(condition).find(get<0>(i));
      std::size_t b = ItemToString(condition).find(get<2>(i));
      if(a != std::string::npos && b != std::string::npos){
        selectivity = get<3>(i);
      }
      return selectivity;
    }
    return selectivity;
}