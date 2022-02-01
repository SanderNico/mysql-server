#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <vector>
#include <tuple>
#include "selectivity_reader.h"
#include "sql/tuple_struct.h"

using std::string;

void GetSelectivitiesFromFile(string Filepath){
    string line, word, tuplestring;


    std::fstream file(Filepath, std::ios::in);
    if(file.is_open()){
        while(getline(file, line)){
            std::stringstream str(line);

            // InMemorySelectivityTable->AddRow(line.substr(0,line.find(",")), 
            //                                         line.substr(1,line.find(",")),
            //                                         line.substr(2,line.find(",")),
            //                                         stod(line.substr(3,line.find(","))));
        }
    }
}