#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <intrin.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    fnEagleVMBegin();

    std::string key;
    std::cout << "license key: ";
    std::getline(std::cin, key);

    if (key.size() < 20)
    {
        printf("invalid key :(\n");
        return 0;
    }

    char* key_buf = key.data();

    // appears to crash here
    bool authed = false;

    if (key == "1337") {
        authed = true;
    }

    if (authed == true) {
        printf("correct key\n");
    }

    int odd_sum = 0;
    int even_sum = 0;
    for (int i = 0; i < 20; i++)
    {
        int ia = key_buf[i] - '0';
        if (i % 2)
        {
            even_sum += ia;
        }
        else
        {
            odd_sum += ia;
        }
    }

    if (odd_sum == 25 && even_sum == 60)
    {
        printf("congradulations, you earned a cookie!\n");
    }
    else
    {
        printf("almost...\n");
    }

    fnEagleVMEnd();
    return 0;
}