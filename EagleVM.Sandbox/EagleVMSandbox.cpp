#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <intrin.h>
#include <iostream>
#include <string>

int main(int, char*[])
{
    fnEagleVMBegin();

    std::string some_output;

    auto total_sum = 0;
    for (auto i = 0; i < 255; i++)
    {
        total_sum += i;
        for (int j = 0 ; j < i; j++)
        {
            total_sum += i;
            some_output += std::to_string(total_sum);
        }

        std::reverse(some_output.begin(), some_output.end());
    }

    std::cout << some_output << std::endl;
    system("pause");

    fnEagleVMEnd();
    return 0;
}