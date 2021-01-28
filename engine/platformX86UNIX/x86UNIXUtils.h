//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#ifndef _X86UNIXUTILS_H_
#define _X86UNIXUTILS_H_

struct utsname;

class UnixUtils
{
public:
   UnixUtils();
   virtual ~UnixUtils();

   /**
      Returns true if we're running in the background, false otherwise.
      There's no "standard" way to determine this in unix, but
      modern job control unices should support the method described
      here:

      http://www.faqs.org/faqs/unix-faq/faq/part3/

      (question 3.7)
    */
   bool inBackground();

   /**
      Returns the name of the OS, as reported by uname.
    */
   const char* getOSName();

private:
   struct utsname* mUnameInfo;
};

extern UnixUtils *UUtils;

// utility class for running a unix command and capturing its output
class UnixCommandExecutor
{
   private:
      int mRet;
      int mStdoutSave;
      int mStderrSave;
      int mPipeFiledes[2];
      int mChildPID;
      int mBytesRead;
      bool mStdoutClosed;
      bool mStderrClosed;
      bool mChildExited;

      void clearFields();
      void cleanup();

   public:
      UnixCommandExecutor();
      ~UnixCommandExecutor();

      // Runs the specified command.
      // - args is a null terminated list of the command and its arguments,
      // e.g: "ps", "-aux", NULL
      // - stdoutCapture is the buffer where stdout data will be stored
      // - stdoutCaptureSize is the size of the buffer
      // None of these parameters may be null.  stdoutCaptureSize must be > 0
      //
      // returns -2 if the parameters are bad.  returns -1 if some other
      // error occurs, check errno for the exact error.
      int exec(char* args[], char* stdoutCapture, int stdoutCaptureSize);
};

#endif
