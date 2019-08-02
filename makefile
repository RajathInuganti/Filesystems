File: File.o
	gcc File.o -lm -o File



File.o: File.c
	gcc -c File.c -o File.o -Wall -Werror
