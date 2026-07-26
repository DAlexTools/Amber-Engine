#include "Editor/Shell/EditorApplication.h"

#include <cstring>
#include <filesystem>
#include <iostream>

namespace
{
    void PrintUsage()
    {
        std::cout
            << "Usage:\n"
            << "  AmberEditor.exe [Project.amberproject]\n"
            << "  AmberEditor.exe --project Project.amberproject\n"
            << "  AmberEditor.exe --smoke-test [--project Project.amberproject]\n";
    }
}

int main(int argc, char** argv)
{
    bool smokeTest = false;
    std::filesystem::path projectFile;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--smoke-test") == 0)
        {
            smokeTest = true;
        }
        else if (std::strcmp(argv[i], "--project") == 0)
        {
            if (i + 1 >= argc)
            {
                std::cerr << "--project requires a .amberproject path." << std::endl;
                PrintUsage();
                return 2;
            }
            projectFile = argv[++i];
        }
        else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0)
        {
            PrintUsage();
            return 0;
        }
        else if (argv[i][0] == '-')
        {
            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            PrintUsage();
            return 2;
        }
        else if (projectFile.empty())
        {
            projectFile = argv[i];
        }
        else
        {
            std::cerr << "Only one .amberproject path can be opened at startup." << std::endl;
            PrintUsage();
            return 2;
        }
    }

    AE::Editor::EditorApplication editor;
    if (smokeTest)
    {
        return editor.RunSmokeTest(projectFile) ? 0 : 1;
    }

    return editor.Run(projectFile);
}
