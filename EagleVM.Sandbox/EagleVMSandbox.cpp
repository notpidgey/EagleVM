#include "EagleVMStub/EagleVMStub.h"
#include <cstdio>
#include <intrin.h>
#include <iostream>
#include <string>
#include <cstdlib>  // For srand() and rand()
#include <ctime>    // For time()

int main(int argc, char* argv[])
{
    fnEagleVMBegin();

    srand(time(0)); // Seed for random number generation

    // Generate a random key
    std::string correctKey = std::to_string(rand());

    std::string key;
    std::cout << "license key: ";
    std::cin >> key;

    // Compare user input with the generated key
    if (key == correctKey) {
        std::cout << "\ncorrect key";
    } else {
        std::cout << "\nincorrect key";
    }

    fnEagleVMEnd();
    return 0;
}
