all:
	gcc ../main.c -I ../inc -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o templedriver

install:
	cp templedriver $(DESTDIR)

uninstall:
	rm $(DESTDIR)/templedriver

clean:
	rm templedriver
