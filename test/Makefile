CFLAGS=-DDEBUG -DUNDER_TEST -fsanitize=undefined -fstack-protector-strong -fstack-protector-all -std=c++1y -Wall -g -O0 -fprofile-arcs -ftest-coverage -fno-elide-constructors
OBJ=Packet.o Buffer.o Task.o FakeArduino.o Network.o Callsign.o Params.o CLI.o L4Protocol.o L7Protocol.o Modifier.o Proto_Ping.o Proto_Rreq.o Modf_Rreq.o Modf_R.o Proto_Beacon.o Proto_C.o Proto_HMAC.o sha256.o HMACKeys.o Proto_Switch.o NVRAM.o Preferences.o Timestamp.o Console.o Serial.o CryptoKeys.o BlockCipher.o AES256.o AESCommon.o Crypto.o

all: test testnet testnet2

clean:
	rm -rf *.o test testnet *.gcda *.gcno *.info out *.dSYM *.log *.val *.gcov

.cpp.o: *.h
	gcc $(CFLAGS) -c $<

test: test.cpp $(OBJ) *.h
	gcc $(CFLAGS) -o test test.cpp $(OBJ) -lstdc++

testnet: testnet.cpp $(OBJ) *.h
	gcc $(CFLAGS) -o testnet testnet.cpp $(OBJ) -lstdc++

testnet2: testnet2.cpp $(OBJ) *.h
	gcc $(CFLAGS) -o testnet2 testnet2.cpp $(OBJ) -lstdc++

recov:
	rm -f *.gcda

coverage:
	rm -f *.info
	gcov test.cpp
	gcov testnet.cpp
	gcov testnet2.cpp
	lcov -c --directory . --output-file main_coverage.info
	lcov -r main_coverage.info '*/RS-FEC.h' -o main_coverage.info
	lcov -r main_coverage.info '*/testnet.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/testnet2.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/FakeArduino.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/Serial.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/Preferences.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/sha256.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/Crypto.h' -o main_coverage.info
	lcov -r main_coverage.info '*/Crypto.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/AESCommon.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/AES256.cpp' -o main_coverage.info
	lcov -r main_coverage.info '*/BlockCipher.cpp' -o main_coverage.info
	genhtml main_coverage.info --output-directory out
	xdg-open out/index.html
