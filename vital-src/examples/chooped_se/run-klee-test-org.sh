#!/bin/bash
#test_file=$1
test_file=test-org.c
clang -g -O0 -emit-llvm -c ./$test_file -o $test_file.bc
llvm-dis $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --only-output-states-covering-new --search=random-path test.bc
# klee --libc=uclibc --write-paths --posix-runtime --emit-all-errors --search=mcts --simulation-search=random-path $test_file.bc
klee --libc=uclibc --emit-all-errors --posix-runtime --search=mcts --simulation-search=dfs \
    --package-name=test-org $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --search=mcts --simulation-search=dfs $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --emit-all-errors --search=mcts --simulation-search=dfs $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --emit-all-errors --search=dfs $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --emit-all-errors --search=random-path $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --emit-all-errors --search=random-state $test_file.bc
# klee --libc=uclibc --write-paths --posix-runtime --emit-all-errors --search=bfs $test_file.bc
