#!/bin/sh 

# This tool is used to populate the "tplib" directory for a linux binary
# package.
# See the "Building a Linux Binary Package" document for details.


if [ "$1" == "" ] || [ "$2" == "" ]
then
    echo "Usage: mktplib.sh path/to/openal path/to/game_executable [ path/to/tplib_template ]"
    exit 1
fi

OPENAL_FILE=$1/linux/src/libopenal.so.0.0.6
GAME_EXE=$2
GAME_ROOT=`dirname $2`
TPLIB_ROOT=$GAME_ROOT/tplib 
TPLIB_TEMPLATE=$3

if [ ! -f "$OPENAL_FILE" ]
then
   echo "error: $OPENAL_FILE does not exist"
   exit 1
fi

if [ ! -d "$GAME_ROOT" ]
then
   echo "error: $GAME_ROOT is not a directory"
   exit 1
fi

if [ ! -x "$GAME_EXE" ]
then
   echo "error: $GAME_EXE is not an executable file"
   exit 1
fi

if [ "$TPLIB_TEMPLATE" == "" ]
then
    if [ -d "tplib_template" ]
    then
        TPLIB_TEMPLATE="tplib_template"
    elif [ -d "tools/unix/tplib_template" ] 
    then
        TPLIB_TEMPLATE="tools/unix/tplib_template"
    else
        echo "error: can't find tplib_template directory, please specify it on command line"
        exit 1
    fi
fi

if [ ! -d "$TPLIB_TEMPLATE" ]
then
    echo "error: $TPLIB_TEMPLATE is not a directory"
    exit 1
fi

if [ ! -d "$TPLIB_ROOT" ]
then
    mkdir -p $TPLIB_ROOT
fi

#PRECMD=echo

$PRECMD rm -rf $TPLIB_ROOT/*
$PRECMD cp $OPENAL_FILE $TPLIB_ROOT/libopenal.so
$PRECMD cp `ldd $GAME_EXE | awk '{print $3}' | grep -v "libpthread\.so" | grep -v "libdl\.so" | grep -v "libm\.so" | grep -v "libc\.so" | grep -v "ld-linux\.so"` $TPLIB_ROOT
$PRECMD cp -r `ls -d $TPLIB_TEMPLATE/* | grep -v "CVS"` $TPLIB_ROOT
