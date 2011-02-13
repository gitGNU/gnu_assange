assange : assange.o assange-jessica.o
	gcc -o assange -lc assange.o assange-jessica.o

assange.o : assange.c assange-log.h assange-jessica.h assange-common.h
	gcc -c -x c -o assange.o assange.c

assange-jessica.o : assange-jessica.c assange-jessica.h assange-common.h
	gcc -c -x c -o assange-jessica.o assange-jessica.c

.PHONEY : clean

clean :
	-rm assange.o
	-rm assange-jessica.o
	-rm assange

