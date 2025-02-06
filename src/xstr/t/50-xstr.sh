#!/bin/sh

gcc -I. -It/files t/files/xstr.c xstr.o -o "$RUNDIR/xstr" &&
"$RUNDIR/xstr"
