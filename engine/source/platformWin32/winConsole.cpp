//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "platformWin32/winConsole.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "core/unicode.h"
#include "console/consoleTypes.h"

WinConsole* WindowsConsole = NULL;

namespace Con
{
    extern bool alwaysUseDebugOutput;
}

ConsoleFunction(enableWinConsole, void, 2, 3, "enableWinConsole(enable, [showWindow])")
{
    argc;

    bool showWindow = true;
    if (argc > 2)
        showWindow = dAtob(argv[2]);

    WindowsConsole->enable(dAtob(argv[1]), showWindow);
}

void WinConsole::create()
{
    WindowsConsole = new WinConsole();
}

void WinConsole::destroy()
{
    delete WindowsConsole;
    WindowsConsole = NULL;
}

void WinConsole::enable(bool enabled, bool showWindow)
{
    winConsoleEnabled = enabled;
    winShowWindow = showWindow;
    if (winConsoleEnabled)
    {
        if (winShowWindow)
        {
            AllocConsole();
            const char *title = Con::getVariable("Con::WindowTitle");
            if (title && *title)
            {
#ifdef UNICODE
                UTF16 buf[512];
                convertUTF8toUTF16((UTF8 *) title, buf, sizeof(buf));
                SetConsoleTitle(buf);
#else
                SetConsoleTitle(title);
#endif
            }
            stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            stdIn = GetStdHandle(STD_INPUT_HANDLE);
            stdErr = GetStdHandle(STD_ERROR_HANDLE);

            printf("%s", Con::getVariable("Con::Prompt"));
        } else {
            stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            stdErr = GetStdHandle(STD_ERROR_HANDLE);
        }
    }
}

bool WinConsole::isEnabled()
{
    if (WindowsConsole)
        return WindowsConsole->winConsoleEnabled;

    return false;
}

static void winConsoleConsumer(ConsoleLogEntry::Level level, const char* line)
{
    if (WindowsConsole)
    {
        WindowsConsole->processConsoleLine(line);
#ifndef TORQUE_SHIPPING
        // see console.cpp for a description of Con::alwaysUseDebugOutput
        if (level == ConsoleLogEntry::Error || Con::alwaysUseDebugOutput)
        {
            OutputDebugStringA(line);
            OutputDebugStringA("\n");
        }
#endif
    }
}

WinConsole::WinConsole()
{
    for (S32 iIndex = 0; iIndex < MAX_CMDS; iIndex++)
        rgCmds[iIndex][0] = '\0';

    iCmdIndex = 0;
    winConsoleEnabled = false;
    Con::addConsumer(winConsoleConsumer);
    inpos = 0;
    lineOutput = false;
}

WinConsole::~WinConsole()
{
    Con::removeConsumer(winConsoleConsumer);
}

void WinConsole::printf(const char* s, ...)
{
    // Get the line into a buffer.
    static const int BufSize = 4096;
    static char buffer[4096];
    DWORD bytes;
    va_list args;
    va_start(args, s);
    vsnprintf(buffer, BufSize, s, args);
    // Replace tabs with carats, like the "real" console does.
    char* pos = buffer;
    while (*pos) {
        if (*pos == '\t') {
            *pos = '^';
        }
        pos++;
    }
    // Axe the color characters.
    Con::stripColorChars(buffer);
    // Print it.
    WriteFile(stdOut, buffer, strlen(buffer), &bytes, NULL);
    FlushFileBuffers(stdOut);
}

void WinConsole::processConsoleLine(const char* consoleLine)
{
    if (winConsoleEnabled)
    {
        inbuf[inpos] = 0;
        if (lineOutput || !winShowWindow)
            printf("%s\n", consoleLine);
        else
            printf("%c%s\n%s%s", '\r', consoleLine, Con::getVariable("Con::Prompt"), inbuf);
    }
}

void WinConsole::process()
{
    if (winConsoleEnabled)
    {
        DWORD numEvents;
        GetNumberOfConsoleInputEvents(stdIn, &numEvents);
        if (numEvents)
        {
            INPUT_RECORD rec[20];
            char outbuf[512];
            S32 outpos = 0;

            ReadConsoleInput(stdIn, rec, 20, &numEvents);
            DWORD i;
            for (i = 0; i < numEvents; i++)
            {
                if (rec[i].EventType == KEY_EVENT)
                {
                    KEY_EVENT_RECORD* ke = &(rec[i].Event.KeyEvent);
                    if (ke->bKeyDown)
                    {
                        switch (ke->uChar.AsciiChar)
                        {
                            // If no ASCII char, check if it's a handled virtual key
                        case 0:
                            switch (ke->wVirtualKeyCode)
                            {
                                // UP ARROW
                            case 0x26:
                                // Go to the previous command in the cyclic array
                                if ((--iCmdIndex) < 0)
                                    iCmdIndex = MAX_CMDS - 1;

                                // If this command isn't empty ...
                                if (rgCmds[iCmdIndex][0] != '\0')
                                {
                                    // Obliterate current displayed text
                                    for (S32 i = outpos = 0; i < inpos; i++)
                                    {
                                        outbuf[outpos++] = '\b';
                                        outbuf[outpos++] = ' ';
                                        outbuf[outpos++] = '\b';
                                    }

                                    // Copy command into command and display buffers
                                    for (inpos = 0; inpos < (S32)strlen(rgCmds[iCmdIndex]); inpos++, outpos++)
                                    {
                                        outbuf[outpos] = rgCmds[iCmdIndex][inpos];
                                        inbuf[inpos] = rgCmds[iCmdIndex][inpos];
                                    }
                                }
                                // If previous is empty, stay on current command
                                else if ((++iCmdIndex) >= MAX_CMDS)
                                {
                                    iCmdIndex = 0;
                                }

                                break;

                                // DOWN ARROW
                            case 0x28: {
                                // Go to the next command in the command array, if
                                // it isn't empty
                                if (rgCmds[iCmdIndex][0] != '\0' && (++iCmdIndex) >= MAX_CMDS)
                                    iCmdIndex = 0;

                                // Obliterate current displayed text
                                for (S32 i = outpos = 0; i < inpos; i++)
                                {
                                    outbuf[outpos++] = '\b';
                                    outbuf[outpos++] = ' ';
                                    outbuf[outpos++] = '\b';
                                }

                                // Copy command into command and display buffers
                                for (inpos = 0; inpos < (S32)strlen(rgCmds[iCmdIndex]); inpos++, outpos++)
                                {
                                    outbuf[outpos] = rgCmds[iCmdIndex][inpos];
                                    inbuf[inpos] = rgCmds[iCmdIndex][inpos];
                                }
                            }
                                     break;

                                     // LEFT ARROW
                            case 0x25:
                                break;

                                // RIGHT ARROW
                            case 0x27:
                                break;

                            default:
                                break;
                            }
                            break;
                        case '\b':
                            if (inpos > 0)
                            {
                                outbuf[outpos++] = '\b';
                                outbuf[outpos++] = ' ';
                                outbuf[outpos++] = '\b';
                                inpos--;
                            }
                            break;
                        case '\t':
                            // In the output buffer, we're going to have to erase the current line (in case
                            // we're cycling through various completions) and write out the whole input
                            // buffer, so (inpos * 3) + complen <= 512.  Should be OK.  The input buffer is
                            // also 512 chars long so that constraint will also be fine for the input buffer.
                        {
                            // Erase the current line.
                            U32 i;
                            for (i = 0; i < inpos; i++) {
                                outbuf[outpos++] = '\b';
                                outbuf[outpos++] = ' ';
                                outbuf[outpos++] = '\b';
                            }
                            // Modify the input buffer with the completion.
                            U32 maxlen = 512 - (inpos * 3);
                            if (ke->dwControlKeyState & SHIFT_PRESSED) {
                                inpos = Con::tabComplete(inbuf, inpos, maxlen, false);
                            }
                            else {
                                inpos = Con::tabComplete(inbuf, inpos, maxlen, true);
                            }
                            // Copy the input buffer to the output.
                            for (i = 0; i < inpos; i++) {
                                outbuf[outpos++] = inbuf[i];
                            }
                        }
                        break;
                        case '\n':
                        case '\r':
                            outbuf[outpos++] = '\r';
                            outbuf[outpos++] = '\n';

                            inbuf[inpos] = 0;
                            outbuf[outpos] = 0;
                            printf("%s", outbuf);

                            S32 eventSize;
                            eventSize = ConsoleEventHeaderSize;

                            dStrcpy(postEvent.data, inbuf);
                            postEvent.size = eventSize + dStrlen(inbuf) + 1;
                            Game->postEvent(postEvent);

                            // If we've gone off the end of our array, wrap
                            // back to the beginning
                            if (iCmdIndex >= MAX_CMDS)
                                iCmdIndex %= MAX_CMDS;

                            // Put the new command into the array
                            strcpy(rgCmds[iCmdIndex++], inbuf);

                            printf("%s", Con::getVariable("Con::Prompt"));
                            inpos = outpos = 0;
                            break;
                        default:
                            inbuf[inpos++] = ke->uChar.AsciiChar;
                            outbuf[outpos++] = ke->uChar.AsciiChar;
                            break;
                        }
                    }
                }
            }
            if (outpos)
            {
                outbuf[outpos] = 0;
                printf("%s", outbuf);
            }
        }
    }
}
