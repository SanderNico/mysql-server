#include <tuple>
#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include "sql/tuple_struct.h"
#include "sql/item.h"

using std::string;


namespace inmemorytuple{
  // double InMemoryTuple::GetSelectivityForCondition(Item *condition){
  //     double selectivity = -1.0;

  //     for(std::vector< tuple<string, string, string, double>>::size_type it = 0; it != Content.size(); it++){
  //       std::size_t a = ItemToString(condition).find(get<0>(Content.at(it)));
  //       std::size_t b = ItemToString(condition).find(get<2>(Content.at(it)));
  //       if(a != std::string::npos && b != std::string::npos){
  //         selectivity = get<3>(Content.at(it));
  //         return selectivity;
  //       }
  //     }
  //     return selectivity;
  // }

  // void InMemoryTuple::SetContent(vector <tuple<string, string, string, double> > PublicContent){
  //   Content = PublicContent;
  // }

  /*
   * TupleContent
   */
  void TupleContent::SetContent(string aExp, string oper, string bExp, double selectivity){
    TupleContent::a = aExp;
    TupleContent::op = oper;
    TupleContent::b = bExp;
    TupleContent::sel = selectivity;
  }

  /*
   * Row
   */
  void Row::AddContent(TupleContent content){
    Row::Tuple = make_tuple(content.a, content.op, content.b, content.sel);
  }

  /*
   * Table
   */
  void Table::AddRow(Row row){
    Table::Rows.emplace_back(row);
  }
}