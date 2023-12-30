#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

#ifdef EAGLEVMSTUB_EXPORTS
#define EAGLEVMSTUB_API __declspec(dllexport)
#else
#define EAGLEVMSTUB_API __declspec(dllimport)
#endif

EAGLEVMSTUB_API void fnEagleVMBegin(void);
EAGLEVMSTUB_API void fnEagleVMEnd(void);