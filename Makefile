#
 # Makefile
 #

all:    libs

libs:   qjump-app-util.so

qjump-app-util.so: qjump-app-util.c
	rm -f qjump-app-util.so*
	gcc -O3 -fPIC -shared -Werror -Wall -o qjump-app-util.so  qjump-app-util.c -ldl

clean:
	rm -f qjump-app-util.so* 
