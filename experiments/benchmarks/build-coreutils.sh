#!/bin/bash

PROG_LIST=${1} # need absolute path
NUM_MAKE_CORES=4

# un-zip
tar -xf coreutils-9.5.tar.gz
cd coreutils-9.5/

# build LLVM bitcode files (without instrumentation, used for measuring coverage)
mkdir obj-llvm
cd obj-llvm
CC=wllvm FORCE_UNSAFE_CONFIGURE=1 CFLAGS="-g -O0 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-nls
make -j${NUM_MAKE_CORES}
cd src
find . -executable -type f | xargs -I '{}' extract-bc '{}'

# build binaries with gcov
# you can parallelize this part with standard parallelization tools like GNU parallel
cd ..
cat ${PROG_LIST} | while read prog
do
    cd ..
    mkdir obj-gcov-${prog}
    cd obj-gcov-${prog}
    ../configure FORCE_UNSAFE_CONFIGURE=1 --disable-nls CFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"
    make -j${NUM_MAKE_CORES}
done