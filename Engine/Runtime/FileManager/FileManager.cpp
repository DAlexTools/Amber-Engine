#include "FileManager.h"
#include <fstream>
#include <stdexcept>


#if defined (_WIN32) || defined  (__linux__) 

class PCFileManager : public FileManager
{
public:
    std::string Read(const std::string& path) override
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open file: " + path);

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }

    void Write(const std::string& path, const std::string& text) override
    {
        std::ofstream file(path, std::ios::trunc | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + path);
        }

        file << text << std::endl;
    }
};

#endif

FileManager& FileManager::Instance()
{
#if defined (_WIN32) || defined (__linux__)
    static PCFileManager instance;
    return instance;
#elif defined (__ANDROID__)
    // todo for android 
    // static AndroidManager instance;
    // return instance;
#else
    throw std::runtime_error("Unsupported platform");
#endif
}


