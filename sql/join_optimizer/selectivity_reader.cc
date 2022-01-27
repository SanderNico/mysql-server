#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <vector>
#include <tuple>
#include "selectivity_reader.h"
#include "sql/tuple_struct.h"

using namespace std;
typedef tuple <string, string, string, double> Row;

vector <tuple <string, string, string, double> > GetSelectivitiesFromFile(string Filepath){
    vector <tuple <string, string, string, double> > Content;
    Row Row;
    string line, word, tuplestring;


    fstream file(Filepath, ios::in);
    if(file.is_open()){
        while(getline(file, line)){
            stringstream str(line);

            while(getline(str, word))
                tuplestring.append(word);
            
            Row = make_tuple(tuplestring.substr(0,tuplestring.find(",")), 
                            tuplestring.substr(1,tuplestring.find(",")),
                            tuplestring.substr(2,tuplestring.find(",")),
                            stod(tuplestring.substr(3,tuplestring.find(","))));

            Content.push_back(Row);
        }
    }
    return Content;
}