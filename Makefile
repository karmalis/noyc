##
# Noyc
#
# Noise generator experiments in C
#
# @file
# @version 0.1

build: clean
	mkdir -p bin
	gcc -ggdb -std=gnu11 -flto -lrt -o bin/noyc src/main.c -I.

run:
	./bin/noyc

clean:
	rm -rf ./bin/*


# end
