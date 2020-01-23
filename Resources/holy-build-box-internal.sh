#!/bin/bash
set -e

# Activate Holy Build Box environment.
source /hbb_exe/activate

set -x

mkdir /tmp/build
cd /tmp/build

# Holy Build Box defines LDFLAGS as "-L/hbb_exe/lib
# -static-libstdc++". The "-L/hbb_exe/lib" option results in linking
# errors "undefined reference" to `std::__once_callable',
# 'std::__once_call' and '__once_proxy'.
export LDFLAGS=-static-libstdc++
unset LDPATHFLAGS
unset SHLIB_LDFLAGS
unset LD_LIBRARY_PATH
unset LIBRARY_PATH

mkdir /tmp/source-writeable

cp -r /source/CMakeLists.txt /tmp/source-writeable/
cp -r /source/Plugin /tmp/source-writeable/
cp -r /source/Resources /tmp/source-writeable/
cp -r /source/UnitTestsSources /tmp/source-writeable/
cp -r /source/WebApplication /tmp/source-writeable/

cmake /tmp/source-writeable \
    -DCMAKE_BUILD_TYPE=$1 -DSTATIC_BUILD=ON \
    -DORTHANC_FRAMEWORK_SOURCE=web \
    -DORTHANC_FRAMEWORK_VERSION=1.5.8 \
    -DCMAKE_INSTALL_PREFIX=/target 

make -j`nproc`

if [ "$1" == "Release" ]; then
    strip ./libOrthancWebViewer.so
fi

make install
