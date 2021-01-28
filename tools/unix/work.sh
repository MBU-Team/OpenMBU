#!/bin/sh
########################################################
# does the "work" of unixchange.sh
########################################################

if [ $# -lt 1 ]
then
   echo "usage: $0 <input file>"
   exit 1
fi

if [ -d "${1}" ]
then
   # we want to exit if the file is a directory
#   echo "error: ${1} is a directory"
   exit 1
fi

## add more extensions here if you don't want a particular type of file changed.
case "${1}" in
   *.obj*/*.exe*/*.dsw*/*.opt*/*.png*/*.jpg*/*.jpeg*/*.gif*/*.dso*/*.dif*/*.tga*/*.raw*/*.wad*/*.dts*/*.dsq*/*.ter*/*.doc*/*.ppm*/*.dsp*/*.lib*/*.LIB*)
           exit 1
           ;;
esac

if [ -x "${1}" ]
then
   # we want to exit if the file is executable
#   echo "error: ${1} is executable
   exit 1
fi

if [ ! -f "${1}" ]
then
   echo "error: ${1} does not exist"
   exit 1
fi

# do the work
sh ./dos2unix.sh "${1}" temp
old_size=`ls -l "${1}" | awk '{ print $5 }'`
new_size=`ls -l temp | awk '{ print $5 }'`
if [ "$new_size" -lt "$old_size" ]
then
#   echo "file changed"
#   echo "Changed ${1}"
   echo "${1}" >> changed_files
   echo -n "C"
   cp temp "${1}"
   rm temp
else
   echo -n "."
fi

#echo "Old size: $old_size"
#echo "New size: $new_size"
