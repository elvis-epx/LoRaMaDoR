#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cstdlib>
#include "Packet.h"
#include "LoRaL2/src/sha256.h"
#include "Proto_HMAC.h"
#include "Network.h"
#include "CLI.h"
#include "HMACKeys.h"
#include "Preferences.h"
#include "NVRAM.h"

void test1()
{
	// test default values of empty NVRAM
	assert(arduino_nvram_beacon_load() == 600);
	arduino_nvram_beacon_save(700);
	assert(arduino_nvram_beacon_load() == 600);
	
	assert(arduino_nvram_callsign_load() == "FIXMEE-1");

	Preferences::putString("callsign", "1Q");
	assert(arduino_nvram_callsign_load() == "FIXMEE-2");
	assert(arduino_nvram_load("kkkk") == "None");
}

// dummy
Ptr<Network> Net(0);

void test2() {
	Buffer tm = "a";
	tm += Buffer("b");
	tm += " c";
	tm += 'd';
	assert(tm.startsWith("ab"));
	assert(!tm.startsWith("ac"));
	assert(tm.startsWith(Buffer("ab")));
	assert(tm.startsWith(Buffer("ab ")));
	assert(!tm.startsWith(Buffer("ac")));
	assert(! tm.startsWith(Buffer("ab cd ")));

	tm = tm + tm + "ef" + 'g';
	printf("### %s\n", tm.c_str());
	assert(tm == "ab cdab cdefg");

	tm = Buffer::millis_to_hms(-1);
	assert(tm == tm);
	assert(tm == "???");
	assert(tm != "??");
	assert(! (tm != tm));
	tm = Buffer::millis_to_hms(0);
	assert(tm == "0:00");
	tm = Buffer::millis_to_hms(30 * 1000);
	assert(tm == "0:30");
	tm = Buffer::millis_to_hms(90 * 1000);
	assert(tm == "1:30");
	tm = Buffer::millis_to_hms(61 * 60 * 1000);
	assert(tm == "1:01:00");
	tm = Buffer::millis_to_hms(25 * 60 * 60 * 1000);
	assert(tm == "1:01:00:00");
	assert(Buffer("aa").compareTo("bb") < 0);
	assert(Buffer("aa").compareTo(Buffer("bb")) < 0);
	assert(Buffer("aa").compareTo("a") > 0);
	assert(Buffer("aa").compareTo(Buffer("a")) > 0);
	assert(Buffer("aa").compareTo("aaa") < 0);
	assert(Buffer("aa").compareTo(Buffer("aaa")) < 0);

	int error;
	printf("---\n");
	Ptr<Packet> p = Packet::decode_l3_test("AAAA<BBBB:133", error);
	assert (!!p);
	assert (p->msg().length() == 0);

	p = Packet::decode_l3_test("AAAA-12<BBBB:133 ee", error);
	assert (!!p);
	assert (strcmp("ee", p->msg().c_str()) == 0);
	assert (p->msg() == "ee");
	assert (strcmp(Buffer(p->to()).c_str(), "AAAA-12") == 0);
	
	assert (!Packet::decode_l3_test("A<BBBB:133", error));
	assert (!Packet::decode_l3_test("AAAA<B:133", error));
	assert (!Packet::decode_l3_test("AAAA:BBBB<133", error));
	assert (!Packet::decode_l3_test("AAAA BBBB<133", error));
	assert (!Packet::decode_l3_test("<BBBB:133", error));
	p = Packet::decode_l3_test("AAAA<BBBB:133,aaa,bbb=ccc,ddd=eee,fff bla", error);
	assert (!!p);

	assert (!Packet::decode_l3_test("AAAA<BBBB:133,aaa,,ddd=eee,fff bla", error));
	assert (!Packet::decode_l3_test("AAAA<BBBB:01 bla", error));
	assert (!Packet::decode_l3_test("AAAA<BBBB:aa bla", error));

	p = Packet::decode_l3_test("AAAA<BBBB:133,A,B=C bla", error);
	assert (!!p);
	Ptr<Packet> q = p->change_msg("bla ble");
	Params d = q->params();
	d.put_naked("E");
	d.put_naked("E");
	d.put("F", "G");
	Ptr<Packet> r = p->change_params(d);
	assert(r->params().has("A"));
	assert(r->params().has("B"));
	assert(r->params().is_key_naked("A"));
	assert(! r->params().is_key_naked("B"));
	assert(r->params().get("B") == "C");

	assert(!q->params().has("E"));
	assert(!q->params().has("F"));

	assert(r->params().has("E"));
	assert(r->params().is_key_naked("E"));
	assert(r->params().has("F"));
	assert(! r->params().is_key_naked("F"));
	assert(strcmp(r->params().get("F").c_str(), "G") == 0);

	assert(strcmp(q->msg().c_str(), "bla ble") == 0);
	assert(strcmp(r->msg().c_str(), "bla") == 0);
}

void test3()
{
	Params params;

	params = Params("1234");
	assert (params.is_valid_with_ident());
	assert (params.ident() == 1234);
	params = Params("1235,abc");
	assert (params.is_valid_with_ident());
	assert (params.ident() == 1235);
	assert(params.has("ABC"));
	printf("ABC=%s\n", params.get("ABC").c_str());
	assert(params.is_key_naked("ABC"));

	params = Params("1236,abc,def=ghi");
	assert (params.is_valid_with_ident());
	assert (params.ident() == 1236);
	assert(params.has("ABC"));
	assert(params.has("DEF"));
	assert(strcmp(params.get("DEF").c_str(), "ghi") == 0);
	assert(params.count() == 2);

	params = Params("def=ghi,1239");
	assert (params.is_valid_with_ident());
	assert (params.ident() == 1239);
	assert (params.count() == 1);
	assert (params.has("DEF"));
	assert (strcmp(params.get("DEF").c_str(), "ghi") == 0);

	assert (!Params("123a").is_valid_with_ident());
	assert (!Params("0123").is_valid_with_ident());
	assert (!Params("abc").is_valid_with_ident());
	assert (!Params("abc=def").is_valid_with_ident());
	assert (Params("abc=def").is_valid_without_ident());
	assert (!Params("123,0bc=def").is_valid_with_ident());
	assert (!Params("123,0bc").is_valid_with_ident());
	assert (!Params("123,,bc").is_valid_with_ident());
	assert (Params("1,abc=def").is_valid_with_ident());
	params = Params("1,2,abc=def");
	assert (params.is_valid_with_ident());
	assert (params.ident() == 2);
	assert (!Params("1,,abc=def").is_valid_with_ident());
	assert (!Params("1,a#c=def").is_valid_with_ident());
	assert (!Params("1,a:c=d ef").is_valid_with_ident());
	assert (!Params("1,ac=d ef").is_valid_with_ident());
	assert (Params("ac=d,e,f=").is_valid_without_ident());
	assert (Params("3,ac=d,e,f=").is_valid_without_ident());
	assert (!Params("3,ac=d,e, f=").is_valid_without_ident());
	assert (!Params("ac$c=d").is_valid_without_ident());
	assert (!Params("AC$c=d").is_valid_without_ident());
	assert (Params("999999,ac=d").is_valid_with_ident());
	assert (!Params("9999999,ac=d").is_valid_with_ident());

	Buffer t1 = Network::gen_random_token(8);
	Buffer t2 = Network::gen_random_token(8);
	assert(t1.length() == 8);
	assert(t2.length() == 8);
	assert(t1 != t2);
	for (int i = 0; i < 8; ++i) {
		assert(isalpha(t1.charAt(i)) || isdigit(t1.charAt(i)));
		assert(isalpha(t2.charAt(i)) || isdigit(t2.charAt(i)));
	}
}

void test4()
{
	Buffer x = "a";
	x.append(NULL, 0);
	x += 'b';
	x += 'c';
	assert(x.charAt(-2) == 'b');
	assert(x.charAt(-4) == -1);
	assert(Buffer("aaa").cut(4).length() == 0);
	assert(Buffer("aaa").cut(-4).length() == 0);
	assert(Buffer("  aaa  ").lstrip() == "aaa  ");
	assert(Buffer("  aaa  ").rstrip() == "  aaa");
	assert(Buffer("  aaa  ").strip() == "aaa");
	assert(Buffer("aaa").substr(5, 1).length() == 0);
	assert(Buffer("aaa").substr(2, 20).length() == 1);

	Vector<Buffer> a;
	a.push_back(Buffer("B"));
	a.push_back(Buffer("C"));
	a.push_back(Buffer("D"));
	a.remov(1);
	assert(a.count() == 2);
	assert(a[0] == "B");
	assert(a[1] == "D");
}

void test5()
{
	Dict<int> a;
	a["B"] = 2;
	a["Z"] = 26;
	a["A"] = 1;
	a["D"] = 4;
	a["G"] = 7;
	a["F"] = 6;

	assert(a["B"] == 2);
	assert(a["Z"] == 26);
	assert(a["A"] == 1);
	assert(a["D"] == 4);
	assert(a["G"] == 7);
	assert(a["F"] == 6);

	assert(a.indexOf("A") == 0);
	assert(a.indexOf("B") == 1);
	assert(a.indexOf("D") == 2);
	assert(a.indexOf("F") == 3);
	assert(a.indexOf("G") == 4);
	assert(a.indexOf("Z") == 5);

	a.remove(Buffer("E"));
	a.remove("A");
	assert(a.indexOf("Z") == 4);
}

int main()
{
	Buffer key = HMACKeys::hash_key("abracadabra");
	printf("%s\n", key.c_str());
	assert(key == "9b801f436eeb78055b4d77d9773bbae5"); // calculated with test_hmac.py
	Buffer hmac = HMACKeys::hmac(key, "BBBBAAAA23Ola");
	assert(hmac == "10d872720ebe"); // calculated with test_hmac.py
	Params d23;
	d23.set_ident(23);
	Packet p23(Callsign(Buffer("BBBB")), Callsign(Buffer("AAAA")), d23, Buffer("Ola"));
	// printf("%s\n", p23.encode_l3().c_str());
	auto p23a = Proto_HMAC_tx(key, p23);
	assert(!!p23a.pkt);
	// printf("%s\n", p23a.pkt->encode_l3().c_str());
	assert(p23a.pkt->params().get("H") == "10d872720ebe");
	assert(Proto_HMAC_rx(key, p23).error);
	assert(!Proto_HMAC_rx(key, *p23a.pkt).error);
	
	d23 = p23a.pkt->params();

	d23.put("H", "");
	auto p23e = p23a.pkt->change_params(d23);
	assert(Proto_HMAC_rx(key, *p23e).error);

	d23.remove("H");
	p23e = p23a.pkt->change_params(d23);
	assert(Proto_HMAC_rx(key, *p23e).error);

	d23.put("H", "0123456789ab");
	p23e = p23a.pkt->change_params(d23);
	assert(Proto_HMAC_rx(key, *p23e).error);

	d23.put("H", "0123456789abcdefg");
	p23e = p23a.pkt->change_params(d23);
	assert(Proto_HMAC_rx(key, *p23e).error);

	d23.put("H", "10d872720ebe");
	p23e = p23a.pkt->change_params(d23);
	assert(!Proto_HMAC_rx(key, *p23e).error);

	assert (!Callsign("Q").is_valid());
	assert (Callsign("QB").is_valid());
	assert (Callsign("QB").is_q());
	assert (!Callsign("QB").is_lo());
	assert (Callsign("QL").is_lo());
	assert (Callsign("QL") == Callsign("ql"));
	assert (Callsign("QL") == Buffer("ql"));
	assert (Callsign("QC").is_valid());
	assert (!Callsign("Q1").is_valid());
	assert (!Callsign("Q-").is_valid());
	assert (!Callsign("qcc").is_valid());
	assert (Callsign("xc").is_valid());
	assert (!Callsign("1cccc").is_valid());
	assert (!Callsign("aaaaa-1a").is_valid());
	assert (!Callsign("aaaaa-01").is_valid());
	assert (!Callsign("a#jskd").is_valid());
	assert (!Callsign("-1").is_valid());
	assert (!Callsign("a-1").is_valid());
	assert (!Callsign("aaaa-1-2").is_valid());;
	assert (!Callsign("aaaa-123").is_valid());

	test3();

	Buffer bb("abcde");
	assert (bb.length() == 5);
	assert (strcmp(bb.c_str(), "abcde") == 0);

	Params d;
	d.put_naked("x");
	d.put("y", "456");
	d.set_ident(123);

	test1();
	test2();
	test4();
	test5();

	Packet plong3(Callsign(Buffer("AAAAAAA-11")), Callsign(Buffer("BBBBBB-22")), d, Buffer("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"));
	Buffer b3 = plong3.encode_l3(200);
	assert(b3.length() == 200);

	printf("Autotest ok\n");
}
