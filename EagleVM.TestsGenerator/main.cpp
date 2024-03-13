#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann-json/json.hpp>

int main(int argc, char* argv[])
{
    auto test_data_path = argc > 1 ? argv[1] : "../deps/x86_test_data/TestData64";

    // loop each file that test_data_path contains
    for (const auto & entry :
        std::filesystem::directory_iterator(test_data_path))
    {
        std::printf("[>] generating tests for: %ls\n", entry.path().c_str());

        // read entry file as string
        std::ifstream file(entry.path());
        std::string content((std::istreambuf_iterator(file)), (std::istreambuf_iterator<char>()));
        nlohmann::json::json data = json::parse(f);

    }

}