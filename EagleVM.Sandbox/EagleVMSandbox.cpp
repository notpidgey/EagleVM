#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <intrin.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::string key;
    std::cout << "license key: ";
    std::getline(std::cin, key);

    if(key.size() < 20)
    {
        printf("invalid key : (");
        return 0;
    }

    const char* key_dat = key.c_str();

    fnEagleVMBegin();

    int sum1 = 0, sum2 = 0, prod1 = 1, prod2 = 1;
    for (int i = 0; i < 20; i++)
    {
        int val = key_dat[i] - '0';
        auto res = i % 2 == 0;

        if (res)
        {
            sum1 += val;
            prod1 *= val;
        }
        else
        {
            sum2 -= val;
            prod2 *= val;
        }
    }

    int result1 = sum1 * prod1;
    int result2 = sum2 * prod2;
    int finalResult = result1 % result2; // Use modulus operation for more complexity

    fnEagleVMEnd();

    if(finalResult == 0)
    {
        printf("congradulations, you earned a cookie!");
    }
    else
    {
        printf("almost...");
    }

    return 0;
}