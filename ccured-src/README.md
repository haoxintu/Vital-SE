# Tutorial of using `ccured`

After installing the tool, copy the instrument file `neschecklib.bc` and the ccured analysis library `libccured.so` to the testing folder (e.g., `examples/from-chopped-se`) and follow the following steps to perform testing.

## 1. Compile source code to the intermediate representation (IR)

For a single test program (e.g., `test.c`), users can get IR by executing

```
$clang -emit-llvm -c test.c -o test.bc
```

For more complex test programs (e.g., GNU Coreutils used in the paper), we recommend users to follow the [official document](http://klee.github.io/tutorials/testing-coreutils/) from KLEE to get their IR code file.

Up to now, we assume users have obtained the test program and compiled it into IR code already. We take the `cat` test program in GNU Coreutils as an example to illustrate the core usabilities of **FastKLEE**.

## 2. Instrument IR for preparing type inference
 
Use the compiler tool-chain `llvm-link` to link the target program `cat.bc` and the instrumentation code `neschecklib.bc` together.

```
$cat run-instrumentation.sh
llvm-link cat.bc neschecklib.bc -o cat-linked.bc
$./run-instrumentation.sh
```
## 3. Type inference and produce CheckList

Use the compiler tool-chain `opt` to load `libccured.so` library and perform type inference analysis. This process will produce a text file `cat-checklist.txt` that records all the *unsafe* pointers in `cat.bc`. Such a checking list file will be used in the following process.
 
```
$cat run-type-inference-system.sh
opt -load libccured.so -nescheck -stats -time-passes < cat-linked.bc >& /dev/null
$./run-type-inference-system.sh
```