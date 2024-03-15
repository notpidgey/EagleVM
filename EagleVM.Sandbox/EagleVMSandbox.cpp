#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <intrin.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::string key;
    std::cout << "license key: ";
    std::cin >> key;

    fnEagleVMBegin();

    if (key == "1798263718265567865") {
        std::cout << "correct key";
    }

    if (key != "1798263718265567865") {
        std::cout << "incorrect key";
    }

    fnEagleVMEnd();
    return 0;
}