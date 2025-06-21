#!/bin/bash

# update the ccured library
# cp ../../build/libnescheck.so .
cp ../../build/libccured.so .

if [ "$#" -ne 1 ] ; then
    echo "Please give the input source file ..."
    exit
fi

echo "Compilation ..."
clang -g -O0 -emit-llvm $1 -c -o target-linked-$1.bc

echo "Instrumentation ..."
llvm-link ./target-linked-$1.bc ./neschecklib.bc -o target-linked-$1-opt.bc

echo "Ccured ..."
opt -load ./libccured.so -nescheck -stats -time-passes < target-linked-$1-opt.bc  # >& $1-nescheckout.txt # /dev/null

echo "Check output ..."
mv  recordings.txt $1-checklist.txt
cat $1-checklist.txt
