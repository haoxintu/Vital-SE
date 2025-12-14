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
git clone https://github.com/haoxintu/Vital-SE
mkdir build-vital-se
cd build-vital-se
cmake \
    -DENABLE_SOLVER_STP=ON \
    -DENABLE_POSIX_RUNTIME=ON \
    -DKLEE_UCLIBC_PATH=<klee_uclibc_dir> \
    -DLLVM_CONFIG_BINARY=<llvm_build_dir>/bin/llvm-config \
    -DLLVMCC=<llvm_build_dir>/bin/clang \
    -DLLVMCXX=<llvm_build_dir>/bin/clang++ ../Vital-SE/vital-src
make -j12