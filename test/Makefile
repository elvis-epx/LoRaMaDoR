CFLAGS=-DDEBUG -DUNDER_TEST -fsanitize=undefined -fstack-protector-strong -fstack-protector-all -std=c++1y -Wall -g -O0 -fprofile-arcs -ftest-coverage -fno-elide-constructors

OBJ=Packet.o Buffer.o Task.o FakeArduino.o Network.o Callsign.o Params.o CLI.o L4Protocol.o L7Protocol.o Modifier.o Proto_Ping.o Proto_Rreq.o Modf_Rreq.o Modf_R.o Proto_Beacon.o Proto_C.o Proto_HMAC.o sha256.o

all: test testnet

clean:
	rm -rf *.o test testnet *.gcda *.gcno *.info out *.dSYM *.log *.val

.cpp.o: *.h
	gcc $(CFLAGS) -c $<

test: test.cpp $(OBJ) *.h
	gcc $(CFLAGS) -o test test.cpp $(OBJ) -lstdc++

testnet: testnet.cpp $(OBJ) *.h
	gcc $(CFLAGS) -o testnet testnet.cpp $(OBJ) -lstdc++

recov:
	rm *.gcda

coverage:
	gcov test.cpp
	gcov testnet.cpp
	lcov -c --directory . --output-file main_coverage.info
	genhtml main_coverage.info --output-directory out
	xdg-open out/index.html
