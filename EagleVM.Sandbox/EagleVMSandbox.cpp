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
    std::cin >> key;

    if (key == "1798263718265567865") {
        std::cout << "\ncorrect key";
    }

    if (key != "1798263718265567865") {
        std::cout << "\nincorrect key";
    }

    system("pause");

    fnEagleVMEnd();
    return 0;
}