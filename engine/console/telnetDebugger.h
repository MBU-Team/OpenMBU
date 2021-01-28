//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TELNETDEBUGGER_H_
#define _TELNETDEBUGGER_H_

class CodeBlock;

/// Telnet debug service implementation.
///
/// This is the C++ side of the built-in Torque debugger.
///
/// To use the debugger, use dbgSetParameters(port, password); in the console
/// of the server to enable debugger connections.  Then on some other system,
/// start up the app (you don't have to start a game or connect to the
/// server) and exec("common/debugger/debugger.cs"); in the console.  Then use
/// the debugger GUI to connect to the server with the right port and password.
///
/// @see http://www.planettribes.com/tribes2/editing.shtml for more thorough discussion.
class TelnetDebugger
{
   S32 mAcceptPort;
   NetSocket mAcceptSocket;
   NetSocket mDebugSocket;

   enum {
      
      // We should only change this is we truely 
      // break the protocol in a future version.
      Version = 2,

      PasswordMaxLength = 32,
      MaxCommandSize = 2048
   };

   char mDebuggerPassword[PasswordMaxLength+1];
   enum State
   {
      NotConnected,
      PasswordTry,
      Initialize,
      Connected
   };
   S32 mState;
   char mLineBuffer[MaxCommandSize];
   S32 mCurPos;
   bool mWaitForClient;

   TelnetDebugger();
   ~TelnetDebugger();

   struct Breakpoint
   {
      StringTableEntry fileName;
      CodeBlock *code;
      U32 lineNumber;
      S32 passCount;
      S32 curCount;
      char *testExpression;
      bool clearOnHit;
      Breakpoint *next;
   };
   Breakpoint *mBreakpoints;

   Breakpoint **findBreakpoint(StringTableEntry fileName, S32 lineNumber);

   bool mProgramPaused;
   bool mBreakOnNextStatement;
   S32 mStackPopBreakIndex;

   void addVariableBreakpoint(const char *varName, S32 passCount, const char *evalString);
   void removeVariableBreakpoint(const char *varName);
   void addBreakpoint(const char *fileName, S32 line, bool clear, S32 passCount, const char *evalString);
   void removeBreakpoint(const char *fileName, S32 line);
   void removeAllBreakpoints();

   void debugBreakNext();
   void debugContinue();
   void debugStepIn();
   void debugStepOver();
   void debugStepOut();
   void evaluateExpression(const char *tag, S32 frame, const char *evalBuffer);
   void dumpFileList();
   void dumpBreakableList(const char *fileName);
   void removeBreakpointsFromCode(CodeBlock *code);

   void checkDebugRecv();
   void processLineBuffer(S32);
   void breakProcess();
   void setBreakOnNextStatement( bool enabled );
public:
   static void create();
   static void destroy();

   void process();
   void popStackFrame();
   void pushStackFrame();
   void addAllBreakpoints(CodeBlock *code);

   virtual void executionStopped(CodeBlock *code, U32 lineNumber);
   void send(const char *s);
   void setDebugParameters(S32 port, const char *password, bool waitForClient);
   void processConsoleLine(const char *consoleLine);
};

extern TelnetDebugger *TelDebugger;

#endif
