all:
	gcc ../main.c ../glad_gl.c ../assets/high/alien.o ../assets/high/bg.o ../assets/high/car.o ../assets/high/cia.o ../assets/high/e1.o ../assets/high/e2.o ../assets/high/track.o ../assets/high/train.o ../assets/high/terry.o -I inc -Ofast -lglfw -lasound -pthread -lm -o templedriver

install:
	cp templedriver $(DESTDIR)

uninstall:
	rm $(DESTDIR)/templedriver

clean:
	rm templedriver