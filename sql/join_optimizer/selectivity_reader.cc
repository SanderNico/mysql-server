#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <vector>
#include <tuple>
#include "selectivity_reader.h"

using std::string;

std::vector<std::tuple<string, string, string, double>> GetSelectivitiesFromFile(string Filepath){
    string line, word, tuplestring;

    std::vector<std::tuple<string, string, string, double>> rows;
    std::vector<string> row;

    string a, op, b;
    double sel;

    std::fstream file(Filepath, std::ios::in);
    if(file.is_open()){
        while(std::getline(file, line)){
            row.clear();

            std::stringstream str(line);

            while(std::getline(str, word, ',')){
                row.push_back(word);
            }

            a = row[0];
            op = row[1];
            b = row[2];
            sel = std::stod(row[3]);

            rows.emplace_back(std::make_tuple(a, op, b, sel));
        }
    }
    return rows;
}