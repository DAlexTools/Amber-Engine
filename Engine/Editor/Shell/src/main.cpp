#include "Editor/Shell/EditorApplication.h"

#include <cstring>

int main(int argc, char** argv)
{
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--smoke-test") == 0)
        {
            smokeTest = true;
        }
    }

    AE::Editor::EditorApplication editor;
    if (smokeTest)
    {
        return editor.RunSmokeTest() ? 0 : 1;
    }

    return editor.Run();
}
