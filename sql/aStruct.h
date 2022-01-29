#ifndef TUPLE_STRUCT_H
#define TUPLE_STRUCT_H

#include <sys/types.h>
#include <string>
#include <vector>

struct aStruct{
    public:
        aStruct();
        static int testCount;

        static int getCount();
        static void setCount(int count);
};


#endif /* TUPLE_STRUCT_H */