#define EAGLEVMSTUB_EXPORTS
#include "EagleVMStub/EagleVMStub.h"

EAGLEVMSTUB_API void __stdcall fnEagleVMBegin(void)
{
#ifdef RELEASE
    MessageBoxA(0, "Application running in unprotected mode.", "EagleVM", 0);
    exit(-1);
#endif
}

EAGLEVMSTUB_API void __stdcall fnEagleVMEnd(void)
{
#ifdef RELEASE
    MessageBoxA(0, "Application running in unprotected mode.", "EagleVM", 0);
    exit(-1);
#endif
}