#include <tuple>
#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include "sql/tuple_struct.h"
#include "sql/item.h"

using std::string;


namespace inmemoryselectivitytable{
  /*
   * Row
   */
  std::tuple<string, string, string, double> Row::Tuple;

  void Row::AddContent(string a, string op, string b, double sel){
    Row::Tuple = std::make_tuple(a, op, b, sel);
  }

  /*
   * Table
   */
  std::vector<Row> Table::Rows;

  void Table::AddRow(Row row){
    Table::Rows.emplace_back(row);
  }

  void Table::AddRow(string a, string op, string b, double sel){
    Row row;
    row.AddContent(a, op, b, sel);
    Table::AddRow(row);
  }

  double Table::GetSelectivityForCondition(Item *condition){
    return 0.036;
  }

  // double Table::GetSelectivityForCondition(Item *condition){
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

}