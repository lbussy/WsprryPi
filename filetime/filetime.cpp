// g++ -std=c++17 -lstdc++fs filetime.cpp -o filetime

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    fs::file_time_type file_time = fs::last_write_time("./test.ini");

    while (true)
    {
        if (file_time != fs::last_write_time("./test.ini"))
        {
            std::cout << "File changed" << std::endl;
            file_time = fs::last_write_time("./test.ini");
        }
    }
}
