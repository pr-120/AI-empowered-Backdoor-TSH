bme68x:main.o bme68x.o
	gcc -Wall -o bme68x main.o bme68x.o -lwiringPi -std=gnu99
main.o: main.c bme68x.h bme68x_defs.h
	gcc -Wall -c main.c -lwiringPi -std=gnu99
bme68x.o: bme68x.c bme68x.h bme68x_defs.h
	gcc -Wall -c bme68x.c -std=gnu99
clean:
	rm main.o bme68x.o bme68x