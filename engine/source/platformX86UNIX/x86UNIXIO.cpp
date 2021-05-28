//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platformX86UNIX/platformX86UNIX.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>

int x86UNIXOpen(const char *path, int oflag)
{
   return open(path, oflag, 0666);
}

int x86UNIXClose(int fd)
{
   return close(fd);
}

ssize_t x86UNIXRead(int fd, void *buf, size_t nbytes)
{
   return read(fd, buf, nbytes);
}

ssize_t x86UNIXWrite(int fd, const void *buf, size_t nbytes)
{
   return write(fd, buf, nbytes);
}
