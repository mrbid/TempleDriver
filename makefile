all:
	mkdir -p release
	cc main.c -I inc -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o release/TempleDriver
	strip --strip-unneeded release/TempleDriver
	upx --lzma --best release/TempleDriver

test:
	cc main.c -I inc -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/TempleDriver_test
	/tmp/TempleDriver_test
	rm /tmp/TempleDriver_test

clean:
	rm -f release/TempleDriver
