#include "v8.h"

#include "./src/supangle.hpp"

int main(int argc, char *argv[])
{
    char *filename = argv[1];
    auto *supangle = new Supangle();
    std::unique_ptr<v8::Platform> platform =
        supangle->initializeV8(argc, argv);

    supangle->initializeVM();
    supangle->InitializeProgram(filename);
    supangle->Shutdown();

    return 0;
}