#!/bin/sh

gcc -DTEST_FAIL -I. -It/files t/files/xstr.c xstr.o -o "$RUNDIR/xstr" || exit 2
"$RUNDIR/xstr"
