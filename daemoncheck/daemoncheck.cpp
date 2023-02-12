#include <iostream>
#include <filesystem>
#include <string>

std::string getFileName(std::string filePath, bool withExtension = true, char seperator = '/')
{
    // Get last dot position
    std::size_t dotPos = filePath.rfind('.');
    std::size_t sepPos = filePath.rfind(seperator);
    if(sepPos != std::string::npos)
    {
        return filePath.substr(sepPos + 1, filePath.size() - (withExtension || dotPos != std::string::npos ? 1 : dotPos) );
    }
    return "";
}

bool isDaemon(std::string exePath)
{
    std::string fileName;
    fileName = getFileName(exePath);
    fileName.append("_DAEMON");
    return getenv (fileName.c_str());
}

int main (int argc, char** argv)
{
    std::string exePath = argv[0];
    if (isDaemon(exePath))
        std::cout << argv[0] << " is running as a daemon." << std::endl;
    else
        std::cout << argv[0] << " is not running as a daemon." << std::endl;
}
