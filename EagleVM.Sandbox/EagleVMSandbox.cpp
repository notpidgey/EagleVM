#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <intrin.h>
#include <iostream>
#include <string>

int main(int, char*[])
{
    fnEagleVMBegin();

    std::string key;
    std::cout << "license key: ";
    std::cin >> key;

    if (key == "1798263718265567865") {
        std::cout << "correct key";
    }

    if (key != "1798263718265567865") {
        std::cout << "incorrect key";
    }

    system("pause");

    fnEagleVMEnd();
    return 0;
}