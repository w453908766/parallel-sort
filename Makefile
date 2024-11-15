
main: Makefile main.c radixsort.c radixsort.h utils.c hex.cpp writeOutput.c
	clang main.c radixsort.c utils.c hex.cpp writeOutput.c -o main -g -flto=thin

test: test.c radixsort.c radixsort.h utils.c Makefile
	clang test.c radixsort.c utils.c -o test -O3

gen_random: gen_random.c Makefile
	clang gen_random.c -o gen_random -O3

clean:
	rm main test gen_random