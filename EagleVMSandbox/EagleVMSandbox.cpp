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

    fnEagleVMBegin();

    // Perform basic arithmetic operations
    const int result1 = chars[0] + chars[4] - chars[8] * chars[12]; // 40 + 50 - 89 * 100
    const int result2 = chars[1] - chars[5] * chars[9] + chars[13]; //
    const int result3 = chars[2] * chars[6] + chars[10] - chars[14];
    const int result4 = chars[3] + chars[7] - chars[11] * chars[15];

    fnEagleVMEnd();

    if(result1 == 100 && result2 == 150 && result3 == 200 && result4 == 250)
    {
        printf("congradulations, you earned a cookie!");
    }
    else
    {
        printf("almost...");
    }

    return 0;
}