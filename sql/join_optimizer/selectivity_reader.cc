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

    std::fstream file(Filepath, std::ios::in);
    if(file.is_open()){
        while(getline(file, line)){
            std::stringstream str(line);

            rows.emplace_back(std::make_tuple(line.substr(0,line.find(",")), 
                                line.substr(1,line.find(",")),
                                line.substr(2,line.find(",")),
                                std::stod(line.substr(3,line.find(",")))));
        }
    }
    return rows;
}