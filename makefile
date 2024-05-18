all:
	mkdir -p release
	cc main.c glad_gl.c assets/high/alien.o assets/high/bg.o assets/high/car.o assets/high/cia.o assets/high/e1.o assets/high/e2.o assets/high/track.o assets/high/train.o assets/high/terry.o -I inc -Ofast -lglfw -lasound -pthread -lm -o release/TempleDriver
	strip --strip-unneeded release/TempleDriver
	upx --lzma --best release/TempleDriver

test:
	cc main.c glad_gl.c assets/high/alien.o assets/high/bg.o assets/high/car.o assets/high/cia.o assets/high/e1.o assets/high/e2.o assets/high/track.o assets/high/train.o assets/high/terry.o -I inc -Ofast -lglfw -lasound -pthread -lm -o /tmp/TempleDriver_test
	/tmp/TempleDriver_test
	rm /tmp/TempleDriver_test

clean:
	rm -f release/TempleDriver
