# set parms by command
if [ $# != 2 ];then
    echo "Please set the time and search strategy to the experiment :)"
    echo " time(min) : e.g., 1h = 60, 12h = 720, 24h = 1440"
    echo " search strategy:  e.g., random-path, dfs, bfs"
    exit 1
fi

time=$1
search=$2

#others: --sym-args 0 1 10 --sym-args 0 2 2 --sym-files 1 8 --sym-stdin 8 --sym-stdout

for bc in {link,ln,logname,ls,md5sum,mkdir,mkfifo,mktemp,mv}
do
klee --simplify-sym-indices --write-cvcs --write-cov --output-module \
--max-memory=1000 --disable-inlining --optimize=false --use-forked-solver \
--use-cex-cache --libc=uclibc --posix-runtime \
--external-calls=all --only-output-states-covering-new \
--env-file=test.env --run-in-dir=/tmp/sandbox \
--max-sym-array-size=4096 --max-solver-time=30s --max-time=$time\min \
--watchdog --max-memory-inhibit=false --max-static-fork-pct=1 \
--max-static-solve-pct=1 --max-static-cpfork-pct=1 --switch-type=internal \
--search=$search --simulation-search=dfs  \
--output-dir="klee-out-$time-min-$search-$bc" --package-name=$bc \
--write-no-tests \
./$bc.bc -sym-args 0 1 10 --sym-args 0 2 2 --sym-files 1 8 --sym-stdin 8 --sym-stdout
done


