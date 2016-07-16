#!/bin/sh

echo "#ifndef ZOIDBERG_VERSION_H"  >zoidberg_version.h
echo "#define ZOIDBERG_VERSION_H" >>zoidberg_version.h
echo "#define ZOIDBERG_BUILD  \"`git show -s --pretty=format:%h`\"" >>zoidberg_version.h
echo "#endif" >>zoidberg_version.h
