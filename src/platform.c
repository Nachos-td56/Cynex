// platform.c
#include "platform.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void cynex_sleep(int ms)
{
    if (ms <= 0) return;

#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((unsigned int)(ms * 1000));   // usleep uses microseconds. thanks linux
#endif
}
