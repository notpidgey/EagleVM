#include "EagleVMStub.h"
#include <cstdio>

int main(int argc, char* argv[])
{
    fnEagleVMBegin();

    int i = 5;
    i++;
    i--;

    int x = 0;
    x += i;

    printf("test %i %i", i, x);

    fnEagleVMEnd();
    return 0;
}
