#! /bin/sh

#
# repair.sh <bad_include>
#
# The purpose of this script is to find .d (Makefile dependency files
# that have bad (old) include files in them and to delete the .d and
# corresponding .o file.

if test $# != 1 || test $1 = "-h" || test $1 = "-help" || test $1 = "--help"; then
  echo ""
  echo "Usage: $0 <bad_include>"
  echo ""
  echo "  bad_include - the name of the bad file (or some unique text"
  echo "                that is grep'd for in the .d files.)"
  echo ""
  echo "This script will run through all the .d (Makefile Dependency)"
  echo "files in the current directory and below.  If any of those files"
  echo "have the 'bad_include' in them, it will delete the .d and"
  echo "corresponding .o file."
  echo ""
  echo "If, for example, you updated to the newest thirdparty (version 1.22)"
  echo "and are now getting:"
  echo ""
  echo "  gmake: No rule to make target ???, needed by ___.o"
  echo ""
  echo "You could run this script like this:"
  echo ""
  echo "cd .../SCIRun/<bin>"
  echo "../src/scripts/repair.sh Thirdparty/1.20"
  echo ""
  echo "And then run your 'gmake' again."
  echo ""
  echo "Example 2:"
  echo ""
  echo "If the error is something like:"
  echo ""
  echo "    No rule to make target `../src/Dataflow/Modules/Render/SCIBaWGL.h',"
  echo "       needed by `Dataflow/Modules/Render/OpenGL.o'.  Stop."
  echo ""
  echo "Then you would use:"
  echo ""
  echo "../src/scripts/repair.sh SCIBaWGL.h"
  echo ""
  exit
fi

bad_inc=$1

files=`find . -name "*.d" -o -name "depend.mk" | xargs grep -l $bad_inc`

file_found=no

for file in $files; do
   file_found=yes

   filename=`echo $file | sed "s%.*/%%"`

   if test "$filename" = "depend.mk"; then
     c_files=`grep $bad_inc $file | cut -f1 -d":"`
     echo "rm $file"
           rm $file
   else
     c_files=$file
   fi

   for cfile in $c_files; do
     base=`echo $cfile | sed "s%\.d%%" | sed "s%\.o%%"`
     echo "rm -rf $base.o"
           rm -rf $base.o
     if test "$filename" != "depend.mk"; then
        echo "rm -rf $base.d"
              rm -rf $base.d
     fi 
   done
done

if test $file_found = "no"; then
   echo ""
   echo "No matching files found. (Perhaps your 'bad_include' was incorrect?)"
   echo "If you continue to have problems with this script, please contact"
   echo "J. Davison de St. Germain (dav@sci.utah.edu)."
   echo ""
fi



