## Build GNU coreutils

We follow the building instructions from KLEE's official website: https://klee-se.org/tutorials/testing-coreutils/

To build it, run `./build-coreutils.sh` in this folder.

## Build CVEs

For each vulnerability, after un-zip `libtasn1-CVEs.zip`, build the library `libtasn1` by executing `make`.
