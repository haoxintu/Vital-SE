# set parms by command
if [ $# != 2 ];then
    echo "Please set the time and search strategy to the experiment :)"
    echo " time(min) : e.g., 1h = 60, 12h = 720, 24h = 1440"
    echo " search strategy:  e.g., random-path, dfs, bfs"
    exit 1
fi

time=$1
search=$2

# Test for the specical programs first

# 1 dd: --sym-args 0 3 10 --sym-files 1 8 --sym-stdin 8 --sym-stdout
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs \
--output-dir="klee-out-$time-min-$search-dd" --package-name=dd \
--write-no-tests \
./dd.bc --sym-args 0 3 10 --sym-files 1 8 --sym-stdin 8 --sym-stdout

# 2 dircolors: --sym-args 0 3 10 --sym-files 2 12 --sym-stdin 12 --sym-stdout
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=dircolors \
--output-dir="klee-out-$time-min-$search-dircolors" \
--write-no-tests \
./dircolors.bc --sym-args 0 3 10 --sym-files 2 12 --sym-stdin 12 --sym-stdout


# 3 echo: --sym-args 0 4 300 --sym-files 2 30 --sym-stdin 30 --sym-stdout
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=echo \
--output-dir="klee-out-$time-min-$search-echo" \
--write-no-tests \
./echo.bc --sym-args 0 4 300 --sym-files 2 30 --sym-stdin 30 --sym-stdout

# 4 expr: --sym-args 0 1 10 --sym-args 0 3 2 --sym-stdout
# klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=expr \
--output-dir="klee-out-$time-min-$search-expr" \
--write-no-tests \
./expr.bc --sym-args 0 1 10 --sym-args 0 3 2 --sym-stdout


# 5 mknod: --sym-args 0 1 10 --sym-args 0 3 2 --sym-files 1 8 --sym-stdin 8 --sym-stdout
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=mknod \
--output-dir="klee-out-$time-min-$search-mknod" \
--write-no-tests \
./mknod.bc --sym-args 0 1 10 --sym-args 0 3 2 --sym-files 1 8 --sym-stdin 8 --sym-stdout


# 6 od: --sym-args 0 3 10 --sym-files 2 12 --sym-stdin 12 --sym-stdout
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=od \
--output-dir="klee-out-$time-min-$search-od" \
./od.bc --sym-args 0 3 10 --sym-files 2 12 --sym-stdin 12 --sym-stdout

# 7 pathchk: --sym-args 0 1 2 --sym-args 0 1 300 --sym-files 1 8 --sym-stdin 8 --sym-stdout
#klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=pathchk \
--output-dir="klee-out-$time-min-$search-pathchk" \
--write-no-tests \
./pathchk.bc --sym-args 0 1 2 --sym-args 0 1 300 --sym-files 1 8 --sym-stdin 8 --sym-stdout


# 8 printf: --sym-args 0 3 10 --sym-files 2 12 --sym-stdin 12 --sym-stdout
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs --package-name=printf  \
--output-dir="klee-out-$time-min-$search-printf" \
--write-no-tests \
./printf.bc --sym-args 0 3 10 --sym-files 2 12 --sym-stdin 12 --sym-stdout




