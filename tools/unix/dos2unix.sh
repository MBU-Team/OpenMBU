#!/bin/sh
########################################################
# converts all of the DOS CR/LF to just UNIX LF.
# arguments:
#  o first argument is the input file to convert
#  o second argument is the output file to write to
########################################################

if [ $# -lt 2 ]
then
   echo "usage: $0 <input file> <output file>"
   exit 1
fi

if [ -d "${1}" ]
then
   # we want to exit if the file is a directory
#   echo "error: ${1} is a directory"
   exit 1
fi

if [ ! -f "${1}" ]
then
   echo "error: ${1} does not exist"
   exit 1
fi

# do the work
cat "${1}" | tr -d '\015' > ${2}
