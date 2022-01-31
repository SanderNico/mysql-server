#include <string>
#include <vector>
#include "aStruct.h"

aStruct::aStruct(){}

int aStruct::getCount(){
    return testCount;
}

void aStruct::setCount(int count){
    testCount = count;
}

aStruct testStruct;

