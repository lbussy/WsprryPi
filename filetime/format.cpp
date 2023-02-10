#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

int main() {
    fs::file_time_type file_time = fs::last_write_time("./test.ini");
    // std::time_t tt = to_time_t(file_time);
    // std::tm *gmt = std::gmtime(&tt);
    // std::stringstream buffer;
    // buffer << std::put_time(gmt, "%A, %d %B %Y %H:%M");
    // std::string formattedFileTime = buffer.str();
    // std::cout << formattedFileTime << '\n';
}
