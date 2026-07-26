#ifndef MYGAME_MODULE_H
#define MYGAME_MODULE_H

namespace MyGame
{

class MyGameModule
{
public:
    void Startup();
    void Shutdown();
    const char* GetName() const;
};

}

#endif
