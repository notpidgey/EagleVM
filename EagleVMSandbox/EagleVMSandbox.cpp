#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::string key_input;
    std::cout << "license key: ";
    std::getline(std::cin, key_input);

    if(key_input.size() <= 20)
    {
        printf("invalid key : (");
        return 0;
    }

    const char* chars = key_input.data();

    fnEagleVMBegin();

    // Perform basic arithmetic operations
    const int result1 = chars[0] + chars[4] - chars[8] * chars[12];
    const int result2 = chars[1] - chars[5] * chars[9] + chars[13];
    const int result3 = chars[2] * chars[6] + chars[10] - chars[14];
    const int result4 = chars[3] + chars[7] - chars[11] * chars[15];

    fnEagleVMEnd();

    if(result1 == 10 && result2 == 15 && result3 == 20 && result4 == 25)
    {
        printf("congradulations, you earned a cookie!");
    }
    else
    {
        printf("almost...");
    }

    return 0;
}