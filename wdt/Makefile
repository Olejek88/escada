wdt: wdt.o 
	g++ wdt.o -o wdt -lpthread -lssl -lz
wdt.o: wdt.c
	g++ -c wdt.c
clean:
	rm -f wdt wdt.o
install:
	cp wdt /usr/local/bin/wdt
uninstall:
	rm -f /usr/local/bin/wdt