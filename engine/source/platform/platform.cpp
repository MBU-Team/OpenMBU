#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"

// The tools prefer to allow the CPU time to process
#ifndef TORQUE_TOOLS
S32 sgBackgroundProcessSleepTime = 25;
#else
S32 sgBackgroundProcessSleepTime = 200;
#endif
S32 sgTimeManagerProcessInterval = 1;

void Platform::initConsole()
{
    Con::addVariable("Pref::backgroundSleepTime", TypeS32, &sgBackgroundProcessSleepTime);
    Con::addVariable("Pref::timeManagerProcessInterval", TypeS32, &sgTimeManagerProcessInterval);
}
