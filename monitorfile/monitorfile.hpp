#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class MonitorFile
{
public:
    void filemon(const std::string &fileName)
    {
        file_name = fileName.c_str();
        std::cout << file_name << std::endl;
        start_monitoring();
    }

    bool changed()
    {
        check_file();
        if (org_time == new_time)
        {
            return false;
        }
        else
        {
            org_time = new_time;
            return true;
        }
    }

private:
    void start_monitoring()
    {
        org_time = fs::last_write_time(file_name);
    }

    void check_file()
    {
        new_time = fs::last_write_time(file_name);
    }

    std::string file_name;
    fs::file_time_type org_time;
    fs::file_time_type new_time;
};
