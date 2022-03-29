/* Copyright (c) 2020, 2021, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "sql/join_optimizer/estimate_selectivity.h"

#include <sys/types.h>
#include <algorithm>
#include <initializer_list>
#include <string>
#include <tuple>

#include "my_bitmap.h"
#include "my_table_map.h"
#include "sql/field.h"
#include "sql/handler.h"
#include "sql/item.h"
#include "sql/item_cmpfunc.h"
#include "sql/item_func.h"
#include "sql/join_optimizer/bit_utils.h"
#include "sql/join_optimizer/print_utils.h"
#include "sql/key.h"
#include "sql/sql_bitmap.h"
#include "sql/sql_const.h"
#include "sql/table.h"
#include "template_utils.h"
#include "sql/tuple_struct.h"
#include "sql/join_optimizer/selectivity_reader.h"
#include "sql/sql_class.h"
#include "sql/count_min_sketch/CountMinSketch.h"

using std::string;

/**
  Estimate the selectivity of (equi)joining a given field to any other field
  using cardinality information from indexes, if possible. Assumes equal
  distribution and zero correlation between the two fields, so if there are
  e.g. 100 records and 4 distinct values (A,B,C,D) for the field, it assumes
  25% of the values will be A, 25% B, etc. (equal distribution), and thus,
  when joining a row from some other table against this one, 25% of the records
  will match (equal distribution, zero correlation).

  If there are multiple ones, we choose the one with the largest
  selectivity (least selective). There are two main reasons for this:

   - Databases generally tend to underestimate join cardinality
     (due to assuming uncorrelated relations); if we're wrong, it would
     better be towards overestimation to try to compensate.
   - Overestimating the number of rows generally leads to safer choices
     that are a little slower for few rows (e.g., hash join).
     Underestimating, however, leads to choices that can be catastrophic
     for many rows (e.g., nested loop against table scans). We should
     clearly prefer the least risky choice here.

  Returns -1.0 if no index was found. Lifted from
  Item_equal::get_filtering_effect.
 */
static double EstimateFieldSelectivity(Field *field, string *trace) {
  const TABLE *table = field->table;
  double selectivity = -1.0;
  for (uint j = 0; j < table->s->keys; j++) {
    if (field->key_start.is_set(j) &&
        table->key_info[j].has_records_per_key(0)) {
      double field_selectivity =
          static_cast<double>(table->key_info[j].records_per_key(0)) /
          table->file->stats.records;
      if (trace != nullptr) {
        *trace +=
            StringPrintf(" - found candidate index %s with selectivity %.10f\n",
                         table->key_info[j].name, field_selectivity);
      }
      selectivity = std::max(selectivity, field_selectivity);
    }
  }

  /*
    Since rec_per_key and rows_per_table are calculated at
    different times, their values may not be in synch and thus
    it is possible that selectivity is greater than 1.0 if
    rec_per_key is outdated. Force the filter to 1.0 in such
    cases.
   */
  return std::min(selectivity, 1.0);
}

/**
  For the given condition, to try estimate its filtering selectivity,
  on a 0..1 scale (where 1.0 lets all records through).

  TODO(sgunders): In some cases, composite indexes might allow us to do better
  for joins with multiple predicates.
 */
double EstimateSelectivity(THD *thd, Item *condition, string *trace) {
  // If the item is a true constant, we can say immediately whether it passes
  // or filters all rows. (Actually, calling get_filtering_effect() below
  // would crash if used_tables() is zero, which it is for const items.)
  if (condition->const_item()) {
    return (condition->val_int() != 0) ? 1.0 : 0.0;
  }

  const bool current_auto_statistics =
      thd->optimizer_switch_flag(OPTIMIZER_SWITCH_AUTO_STATISTICS);
  const bool current_job_selectivities =
      thd->optimizer_switch_flag(OPTIMIZER_SWITCH_JOB_SELECTIVITIES);
  

  if(current_auto_statistics){
    double selectivity = -1;
    if (condition->type() == Item::FUNC_ITEM &&
      down_cast<Item_func *>(condition)->functype() == Item_func::EQ_FUNC) {
      Item_func_eq *eq = down_cast<Item_func_eq *>(condition);
      Item *left = eq->arguments()[0];
      Item *right = eq->arguments()[1];
      if (left->type() == Item::FIELD_ITEM && right->type() == Item::FIELD_ITEM) {
        double estimatedRowsLeft = -1;
        double estimatedRowsRight = -1;

        std::map<std::pair<std::string, std::string>, CountMinSketch>::iterator dict_left;
        std::map<std::pair<std::string, std::string>, CountMinSketch>::iterator dict_right;
        for (Field *field : {down_cast<Item_field *>(left)->field}) {
          auto dict_it = Dictionary.find(std::make_pair(field->table->s->table_name.str, field->field_name));
          dict_left = dict_it;
          if(dict_it != Dictionary.end()){
            estimatedRowsLeft = (double)dict_it->second.totalcount();
          }
        }
        for(Field *field : {down_cast<Item_field *>(right)->field}){
          auto dict_it = Dictionary.find(std::make_pair(field->table->s->table_name.str, field->field_name));
          dict_right = dict_it;
          if(dict_it != Dictionary.end()){
            estimatedRowsRight = (double)dict_it->second.totalcount();
          }
        }

        // AVSLUTTA HER! KJØR DOT-product på hashedROW for alle index? Dobbel for loop

        int estimatedRows = INT_MAX;
        int newEstimate = 0;
        if(dict_left != Dictionary.end() && dict_right != Dictionary.end()){
          int allRowValues [dict_left->second.getDepth()];

          for(unsigned int i = 0; i < dict_left->second.getDepth(); i++){

            int * hashedLeft = dict_left->second.getHashedRow(i);
            int * hashedRight = dict_right->second.getHashedRow(i);

            int newRowValue = 0;

            int rowValue = 0;
            for(unsigned int  it = 0; it < dict_left->second.getWidth(); it++){
              rowValue += hashedLeft[it]*hashedRight[it];

              newRowValue += (hashedLeft[it] - (1/(dict_left->second.getWidth()-1))*(dict_left->second.totalcount()-hashedLeft[it])) *
                              (hashedRight[it] - (1/(dict_right->second.getWidth()-1))*(dict_right->second.totalcount()-hashedRight[it]));
            }

            newRowValue *= ((dict_left->second.getWidth()-1)/dict_left->second.getWidth());

            allRowValues[i] = newRowValue;
            estimatedRows = std::min(estimatedRows, rowValue);
          }

          int n = sizeof(allRowValues) / sizeof(allRowValues[0]);
          std::sort(allRowValues, allRowValues + n);

          newEstimate = allRowValues[dict_left->second.getDepth()/2];


          for(unsigned int iter = 0; iter < dict_left->second.getDepth(); iter++){
            printf("INDEX: %d, ESTIMATED ROWS: %d\n", iter, allRowValues[iter]);
          }
        }



        double newSelectivity = (double) newEstimate / (estimatedRowsLeft*estimatedRowsRight);


        if(estimatedRows != INT_MAX){
          selectivity = (double) estimatedRows / (estimatedRowsLeft*estimatedRowsRight);
        }

        printf("ORIGINAL Selectivity: %.8f, NEW selectivity: %.8f\n", selectivity, newSelectivity);


        //selectivity = std::max((double)-1, std::max(estimatedRowsLeft, estimatedRowsRight)/(estimatedRowsLeft * estimatedRowsRight));
      }else if(left->type() == Item::FIELD_ITEM && !(right->type() == Item::FIELD_ITEM)){
        for(Field *field : {down_cast<Item_field *>(left)->field}){
          auto dict_it = Dictionary.find(std::make_pair(field->table->s->table_name.str, field->field_name));
          if(dict_it != Dictionary.end()){
            // Parsing the predicate, removing '
            std::string parsedPredicate = ItemToString(right);
            parsedPredicate.erase(remove(parsedPredicate.begin(), parsedPredicate.end(), '\''), parsedPredicate.end());

            // Estimate selectivity using countminsketch
            double estimatedRows = (double)dict_it->second.estimate(parsedPredicate.c_str());
            double totalRows = (double)dict_it->second.totalcount();
            selectivity = estimatedRows/totalRows;
          }
        }
      }
    }else if (condition->type() == Item::FUNC_ITEM &&
      down_cast<Item_func *>(condition)->functype() == Item_func::IN_FUNC){
      // For IN - predicates
      Item_func_eq *eq = down_cast<Item_func_eq *>(condition);
      Item *left = eq->arguments()[0];
      Item *right = eq->arguments()[1];
      Field *field = down_cast<Item_field *>(left)->field;

      auto dict_it = Dictionary.find(std::make_pair(field->table->s->table_name.str, field->field_name));
      if(dict_it != Dictionary.end()){
        double estimatedRows = 0;
        double totalRows = dict_it->second.totalcount();

        // Loops through arguments on right side (therefore starting at 1)
        for (unsigned int i = 1; i < eq->arg_count; i++){
          right = eq->arguments()[i];

          // Parsing the predicate, removing '
          std::string parsedPredicate = ItemToString(right);
          parsedPredicate.erase(remove(parsedPredicate.begin(), parsedPredicate.end(), '\''), parsedPredicate.end());
          estimatedRows += (double) dict_it->second.estimate(parsedPredicate.c_str());
       }
       selectivity = estimatedRows/totalRows;
      }
    }
    if (selectivity >= 0.0){
      if (trace != nullptr) {
        *trace +=
          StringPrintf(" - used estimated selectivity for %s, selectivity = %.10f\n",
                      ItemToString(condition).c_str(), selectivity);
      }
      return selectivity;
    }
  }
  
  if(current_job_selectivities){
    if (trace != nullptr) {
              *trace +=
                  StringPrintf(" CONDITION TYPE: %d\n",
                              condition->type());
            }

      //Check if conditions are part of the JOB query
      if(condition->type() == Item::FUNC_ITEM || condition->type() == Item::COND_ITEM){
        double selectivity = -1.0;
        
        selectivity = InMemorySelectivityTable->GetSelectivityForCondition(condition, trace);

        if (selectivity >= 0.0){
          if (trace != nullptr) {
              *trace +=
                  StringPrintf(" - used hardcoded selectivity for %s, selectivity = %.10f\n",
                              ItemToString(condition).c_str(), selectivity);
            }
            return selectivity;
          }
      }
  }

  

  // For field = field (e.g. t1.x = t2.y), we try to use index information
  // to find a better selectivity estimate. We look for indexes on both
  // fields, and pick the least selective (see EstimateFieldSelectivity()
  // for why).
  if (condition->type() == Item::FUNC_ITEM &&
      down_cast<Item_func *>(condition)->functype() == Item_func::EQ_FUNC) {
    Item_func_eq *eq = down_cast<Item_func_eq *>(condition);
    Item *left = eq->arguments()[0];
    Item *right = eq->arguments()[1];

    if (left->type() == Item::FIELD_ITEM && right->type() == Item::FIELD_ITEM) {
      double selectivity = -1.0;
      for (Field *field : {down_cast<Item_field *>(left)->field,
                           down_cast<Item_field *>(right)->field}) {
        selectivity = std::max(selectivity, EstimateFieldSelectivity(field, trace));
      }
      if (selectivity >= 0.0) {
        printf("4, %f\n", selectivity);
        if (trace != nullptr) {
          *trace +=
              StringPrintf(" - used an index for %s, selectivity = %.10f\n",
                           ItemToString(condition).c_str(), selectivity);
        }
        return selectivity;
      }
    }
  }

  // For multi-equalities, we do the same thing. This is maybe surprising;
  // one would think that there are more degrees of freedom with more joins.
  // However, given that we want the cardinality of the join ABC to be the
  // same no matter what the join order is and which predicates we select,
  // we can see that
  //
  //   |ABC| = |A| * |B| * |C| * S_ab * S_ac
  //   |ACB| = |A| * |C| * |B| * S_ac * S_bc
  //
  // (where S_ab means selectivity of joining A with B, etc.)
  // which immediately gives S_ac = S_bc, and similar equations give
  // S_ab = S_ac and so on.
  //
  // So all the selectivities in the multi-equality must be the same!
  // However, if you go to a database with real-world data, you will see that
  // they actually differ, despite the mathematics disagreeing.
  // The mystery, however, is resolved when we realize where we've made a
  // simplification; the _real_ cardinality is given by:
  //
  //   |ABC| = (|A| * |B| * S_ab) * |C| * S_{ab,c}
  //
  // The selectivity of joining AB with C is not the same as the selectivity
  // of joining B with C (since the correlation, which we do not model,
  // differs), but we've approximated the former by the latter. And when we do
  // this approximation, we also collapse all the degrees of freedom, and can
  // have only one selectivity.
  //
  // If we get more sophisticated cardinality estimation, e.g. by histograms
  // or the likes, we need to revisit this assumption, and potentially adjust
  // our model here.
  if (condition->type() == Item::FUNC_ITEM &&
      down_cast<Item_func *>(condition)->functype() ==
          Item_func::MULT_EQUAL_FUNC) {
    Item_equal *equal = down_cast<Item_equal *>(condition);

    // These should have been expanded early, before we get here.
    assert(equal->get_const() == nullptr);

    double selectivity = -1.0;
    for (Item_field &field : equal->get_fields()) {
      selectivity =
          std::max(selectivity, EstimateFieldSelectivity(field.field, trace));
    }
    if (selectivity >= 0.0) {
      if (trace != nullptr) {
        *trace += StringPrintf(" - used an index for %s, selectivity = %.10f\n",
                               ItemToString(condition).c_str(), selectivity);
      }
      return selectivity;
    }
  }

  // Index information did not help us, so use Item::get_filtering_effect().
  //
  // There is a challenge in that the Item::get_filtering_effect() API
  // is intrinsically locked to the old join optimizer's way of thinking,
  // where one made a long chain of (left-deep) nested tables, and selectivity
  // estimation would be run for the entire WHERE condition at all points
  // in that chain. In such a situation, it would be neccessary to know which
  // tables were already in the chain and which would not, and multiple
  // equalities would also be resolved through this mechanism. In the hypergraph
  // optimizer, we no longer have a chain, and always estimate selectivity for
  // applicable conditions only; thus, we need to fake that chain for the API.
  table_map used_tables = condition->used_tables() & ~PSEUDO_TABLE_BITS;
  table_map this_table = IsolateLowestBit(used_tables);
  MY_BITMAP empty;
  my_bitmap_map bitbuf[bitmap_buffer_size(MAX_FIELDS) / sizeof(my_bitmap_map)];
  bitmap_init(&empty, bitbuf, MAX_FIELDS);
  double selectivity = condition->get_filtering_effect(
      thd, this_table, used_tables & ~this_table,
      /*fields_to_ignore=*/&empty,
      /*rows_in_table=*/1000.0);

  if (trace != nullptr) {
    *trace += StringPrintf(" - fallback selectivity for %s = %.10f\n",
                           ItemToString(condition).c_str(), selectivity);
  }
  return selectivity;
}
