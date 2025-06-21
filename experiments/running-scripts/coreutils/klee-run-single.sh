
# set parms by command
if [ $# != 3 ];then
    echo "Please set the time, search strategy and package to the experiment :)"
    echo " time(min) : e.g., 1h = 60, 12h = 720, 24h = 1440"
    echo " search strategy:  e.g., random-path, dfs, bfs"
    echo " package:  e.g., cat"
    exit 1
fi

time=$1
search=$2
bc=$3
#others: --sym-args 0 1 10 --sym-args 0 2 2 --sym-files 1 8 --sym-stdin 8 --sym-stdout


# for bc in {base64,basename,cat,chcon,chgrp,chmod,chown,chroot,cksum,comm}
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--exit-on-error-type=Ptr --error-location=memcpy.c:29 \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --package-name=$bc \
--use-batching-search --batch-instructions=10000 \
--output-dir="klee-out-$time-min-$search-$bc" \
./$bc.bc -sym-args 0 1 10 --sym-args 0 2 2 --sym-files 1 8 --sym-stdin 8 --sym-stdout


