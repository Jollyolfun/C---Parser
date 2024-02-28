compile: compile.c scanner.h scanner.c driver.c
	gcc -Wall -o compile compile.c scanner.h scanner.c driver.c 

clean:
	rm -f *.o compile
