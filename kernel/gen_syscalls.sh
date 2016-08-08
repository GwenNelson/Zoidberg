#!/bin/sh

# Tell me this is a messy hack and i'll agree with you... and then punch you in the face
# You're probably another moronic perl coding bastard aren't you?

cat syscalls.lst | awk '{print "#define ZSYSCALL_" $2 " " $1}' >syscalls.inc
cat syscalls.lst | awk '{print $3 " sys_" tolower($2) " " $4 " " $5" " $6 " "$7" " $8" " $9 " " $10 " " $11 " " $12 }' | sed s/\)/\)\;/ >> syscalls.inc

echo "static void *syscalls["`wc -l < syscalls.lst`"] = {" >>syscalls.inc
echo "NULL," >>syscalls.inc
 cat syscalls.lst  | awk {'print "&sys_" tolower($2) ","'} >> syscalls.inc
echo "};" >> syscalls.inc

