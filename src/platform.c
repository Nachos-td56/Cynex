// platform.c
#include "platform.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

void cynex_sleep(int ms)
{
    if (ms <= 0) return;

#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    // Convert milliseconds to seconds and nanoseconds
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;

    // nanosleep handles interruptions (like signals) gracefully if needed
    // though a simple call is usually sufficient for basic usage
    nanosleep(&ts, NULL);
#endif
}
