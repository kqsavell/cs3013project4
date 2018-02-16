# Kyle Savell & Antony Qin

all: clean p4.c
	gcc -g p4.c -o p4

clean:
	rm -f p4
