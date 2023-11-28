#define EAGLEVMSTUB_EXPORTS
#include "EagleVMStub/EagleVMStub.h"

EAGLEVMSTUB_API void __stdcall fnEagleVMBegin(void)
{
    MessageBoxA(0, "Application running in unprotected mode.", "EagleVM", 0);
}

EAGLEVMSTUB_API void __stdcall fnEagleVMEnd(void)
{
    MessageBoxA(0, "Application running in unprotected mode.", "EagleVM", 0);
}