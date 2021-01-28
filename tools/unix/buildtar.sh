#!/bin/sh

# This tool is used to build a tar file containing a linux binary package.
# See the "Building a Linux Binary Package" document for details.

if [ "$1" == "" ] || [ "$2" == "" ]
then
    echo "Usage: buildtar.sh game_name game_root_dir [release_tag]"
    exit 1
fi

GAME_NAME=$1
GAME_ROOT=$2

if [ ! -d "$GAME_ROOT" ]
then
   echo "error: $GAME_ROOT is not a directory"
   exit 1
fi

if [ "$3" != "" ]
then
    RELEASE_TAG=-$3
fi

DATE=`date +%m-%d-%Y`

# note that the -h on this command includes symlinks as files, not links
tar --exclude CVS -chvjf $GAME_NAME$RELEASE_TAG-$DATE.tar.bz2 $GAME_ROOT
