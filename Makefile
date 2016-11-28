kernel: kernel.o 
	version/version
	g++ kernel.o modbus_slave.o tecon_19.o func.cpp tsrv.o vis-t.o mercury230.o elf2.o irp.o db.o crq.o lk.o km_5.o ce102.o ce303.o panel.o vkt.o karat.o -o kernel -lpthread -lmysqlclient -L/usr/lib -lmysqlclient -lmodbus
kernel.o: kernel.cpp tsrv.cpp mercury230.cpp vis-t.cpp elf2.cpp db.cpp crq.cpp lk.cpp irp.cpp tecon_19.cpp km_5.cpp ce303.cpp ce102.cpp panel.cpp vkt.cpp func.cpp karat.cpp modbus_slave.cpp
	g++ -c modbus_slave.cpp
	g++ -c karat.cpp
	g++ -c vkt.cpp
	g++ -c func.cpp
	g++ -c ce102.cpp
	g++ -c mercury230.cpp
	g++ -c elf2.cpp
	g++ -c ce303.cpp
	g++ -c km_5.cpp
	g++ -c -D_REENTERANT kernel.cpp
	g++ -c -D_REENTERANT crq.cpp
	g++ -c -D_REENTERANT lk.cpp
	g++ -c db.cpp
	g++ -c irp.cpp
	g++ -c tecon_19.cpp
	g++ -c panel.cpp
	g++ -c vis-t.cpp
	g++ -c tsrv.cpp
clean:
	rm -f kernel kernel.o db.o crq.o elf2.o lk.o irp.o tecon_19.o km_5.o ce102.o panel.o vkt.o ce303.o karat.o modbus_slave.o
install:
	cp kernel /usr/local/bin/kernel
uninstall:
	rm -f /usr/local/bin/kernel