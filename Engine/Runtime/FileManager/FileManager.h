#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>


class FileManager
{
public:
    virtual ~FileManager() = default;
    virtual std::string Read(const std::string& path) = 0;
    virtual void Write(const std::string& path, const std::string& text) = 0;

    static FileManager& Instance();

protected:
    FileManager() = default;
};

#endif 
