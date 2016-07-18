#!/bin/sh

echo "#ifndef ZOIDBERG_VERSION_H"  >kernel/zoidberg_version.h
echo "#define ZOIDBERG_VERSION_H" >>kernel/zoidberg_version.h
echo "#define ZOIDBERG_BUILD  \"`git show -s --pretty=format:%h`\"" >>kernel/zoidberg_version.h
echo "#endif" >>kernel/zoidberg_version.h

cat kernel/kernel.inf.template | sed s/@@VERSION@@/`git show -s --pretty=format:%h`/g >kernel/kernel.inf
