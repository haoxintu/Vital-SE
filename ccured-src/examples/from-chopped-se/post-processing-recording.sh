#!/bin/bash

sed -i 's/main/__klee_posix_wrapped_main_nesCheck/g' $1
