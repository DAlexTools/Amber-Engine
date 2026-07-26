#include "MyGameModule.h"

#include <iostream>

int main(int, char**)
{
    MyGame::MyGameModule module;
    module.Startup();
    std::cout << module.GetName() << " launcher started." << std::endl;
    module.Shutdown();
    return 0;
}
