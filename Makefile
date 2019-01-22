CC = gcc

compile:
	@make clean
	@echo Kompilacja pliku
	$(CC) microshell.c -o microshell
clean:
	@echo Usuwamy skompilowane pliki
	@rm -f microshell
	@rm -f /tmp/microShellHistory
run:
	@make compile
	./microshell