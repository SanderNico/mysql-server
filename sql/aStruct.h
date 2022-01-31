#ifndef ASTRUCT_H
#define ASTRUCT_H

#include <string>
#include <vector>

struct aStruct{
    public:
        aStruct();
        static int testCount;

        static int getCount();
        static void setCount(int count);
};

extern aStruct testStruct;

#endif /* TUPLE_STRUCT_H */