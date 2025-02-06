CFLAGS=-I. -It/files/include

test tests:: xstr.o
	sh support/run-tests --keep

xstr.o: xstr.c xstr.h
