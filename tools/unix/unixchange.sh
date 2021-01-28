#!/bin/sh
########################################################
# to execute run 'sh unixchange.sh'
# this sh script will find all of the files that should
# be changed to successfully compile the v12 under Linux
# and will call dos2unix.sh to change those files 
# appropriately
########################################################

echo -n "Searching for files to change: "

find ../../../v12/ -exec sh ./work.sh {} \;

echo
echo "Completed changes successfully."
