##
# Noyc
#
# Noise generator experiments in C
#
# @file
# @version 0.1

build_noyc: clean
	mkdir -p bin
	gcc -ggdb -std=gnu11 -flto -lrt -lm -o bin/noyc src/main.c src/img.c src/iperlin.c -I.

build_noysway: clean
	mkdir -p bin
	gcc -ggdb -std=gnu11 -flto -lrt -lm -lwayland-client -lxkbcommon -o bin/noysway \
		src/noysway.c \
		src/iperlin.c \
		src/wayland/xdg-shell-protocol.c \
		src/sharedmem.c \
	-I.
#		src/wayland/input.c \
		src/wayland/wayland.c \
	-I.

clean:
	rm -rf ./bin/*


# end
