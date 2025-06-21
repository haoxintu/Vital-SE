#!/bin/bash

# install dependencies
sudo apt-get install cmake bison flex libboost-all-dev python perl zlib1g-dev build-essential curl libcap-dev git cmake libncurses5-dev python-minimal python-pip unzip libtcmalloc-minimal4 libgoogle-perftools-dev libsqlite3-dev doxygen
pip3 install tabulate wllvm

# install LLVM-11
wget http://releases.llvm.org/11.0.0/llvm-11.0.0.src.tar.xz
wget http://releases.llvm.org/11.0.0/cfe-11.0.0.src.tar.xz
wget http://releases.llvm.org/11.0.0/clang-tools-extra-11.0.0.src.tar.xz
tar xvf llvm-11.0.0.src.tar.xz
tar xvf cfe-11.0.0.src.tar.xz
tar xvf clang-tools-extra-11.0.0.src.tar.xz
mv llvm-11.0.0.src llvm-src
mv cfe-11.0.0.src clang
mv clang llvm-src/tools/clang
mv clang-tools-extra-11.0.0.src extra 
mv extra llvm-src/tools/clang/tools/extra
cd llvm-src
mkdir build 
cd build
cmake  -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE="Release"  -DCMAKE_INSTALL_PREFIX=./ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
make -j8
make install

# build ccured

cd ccured-src
mkdir build
cd build
cmake ..
make -j12
cd ..
cd ..

# build Vital

## build constraint solver STP
git clone https://github.com/stp/stp.git
cd stp
git checkout tags/2.3.3
mkdir build
cd build
cmake ..
make -j12
sudo make install
cd ..
cd ..

## build uclibc
git clone https://github.com/klee/klee-uclibc.git
cd klee-uclibc
./configure --make-llvm-lib
make -j12
cd ..

## build vital
cd vital-src
mkdir build
cd build
cmake -DENABLE_SOLVER_STP=ON -DENABLE_POSIX_RUNTIME=ON -DENABLE_KLEE_UCLIBC=ON -DKLEE_UCLIBC_PATH=../klee-uclibc \
    -DLLVM_CONFIG_BINARY=/usr/bin/llvm-config-11 -DLLVMCC=/usr/bin/clang-11 -DLLVMCXX=/usr/bin/clang++-11 -DCMAKE_BUILD_TYPE=Debug ..
make -j12
sudo make install
cd ..