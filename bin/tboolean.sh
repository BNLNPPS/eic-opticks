#!/bin/bash -l

arg=${1:-box}
echo ====== $0 $* ====== PWD $PWD ========= arg $arg ========

tboolean-
cmd="tboolean-$arg --okg4 --compute --strace"
echo $cmd
eval $cmd
rc=$?

echo ====== $0 $* ====== PWD $PWD ========= arg $arg ======== RC $rc =======

exit $rc
