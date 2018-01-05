all: traffic

traffic: traffic.o cars.o
	gcc -Wall -g -std=c99 -pthread -o $@ $^

%.o : %.c traffic.h
	gcc -Wall -g -std=c99 -c $<

clean : 
	rm -f *.o traffic *~

