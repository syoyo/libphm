all:
	clang -Weverything -Werror -Wno-padded -Wno-unused-function -O2 -g -o sample libphm.c sample.c
