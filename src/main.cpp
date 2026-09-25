#include "Application.h"

int main()
{
    Application application;
    if (!application.initialize(1280, 720, "Animated 3D Farm Scene"))
    {
        return 1;
    }

    application.run();
    application.shutdown();
    return 0;
}
