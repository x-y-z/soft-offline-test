
CC=gcc

tsoft: tsoft.c 
	$(CC) -o $@ $^
	sudo setcap "all=ep" $@
