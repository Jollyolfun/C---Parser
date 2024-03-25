compile: compile.c scanner.h scanner.c driver.c ast-print.c ast.h
	gcc compile.c scanner.c driver.c ast-print.c -o compile 

clean:
	rm -f *.o compile
