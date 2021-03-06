#!/usr/bin/env python3

import sys, select, time, random
from lora_loop import EventLoop
from lora_conn import Connection
from lora_switch import Switch
from lora_ping import Ping

client_port = int(sys.argv[1])
server_port = int(sys.argv[2])
server_callsign = sys.argv[3]

loop = EventLoop()

print("Connecting...")
client = Connection(client_port)
server = Connection(server_port)
print("Connected")

# Configure TNC mode
client.send("\r!tnc\r")
server.send("\r!tnc\r")

def rst():
	client.send("\r!reset\r")
	server.send("\r!reset\r")
loop.schedule(rst, 60.0)

client.add_proto_handler(["SWC"], Switch)
client.add_proto_handler(["PONG"], Ping)

s = None

def switch_on():
	global s
	s = Switch(client, server_callsign, loop, 1, 1)
	def h(tgt, res):
		print("########", tgt, res)
		assert(res == 1)
		assert(tgt == 1)
	s.on_result(h)
loop.schedule(switch_on, 1.0)

def switch_query():
	s2 = Switch(client, server_callsign, loop, 2)
	def h(tgt, res):
		print("########", tgt, res)
		assert(res == 0)
		assert(tgt == 2)
	s2.on_result(h)
loop.schedule(switch_query, 1.0)

def ping():
	payload = "0123456789" * 10
	payload = payload.encode("ascii")
	p = Ping(client, server_callsign, loop, payload)
	def h(pong_payload):
		print("######## PONG")
		if payload != pong_payload:
			print("Error: payload different")
			print("Len sent: %d" % len(payload))
			print("Len recv: %d" % len(pong_payload))
			print("Payload sent:", payload)
			print("Payload recv:", pong_payload)
			sys.exit(1)
	p.on_result(h)
loop.schedule(ping, 10.0)

def ping2():
	payload = bytearray(random.getrandbits(8) for _ in range(100))
	p = Ping(client, server_callsign, loop, payload)
	def h(pong_payload):
		print("######## PONG")
		if payload != pong_payload:
			print("Error: payload different")
			print("Len sent: %d" % len(payload))
			print("Len recv: %d" % len(pong_payload))
			print("Payload sent:", payload)
			print("Payload recv:", pong_payload)
			sys.exit(1)
			
	p.on_result(h)
loop.schedule(ping2, 20.0)

def switch_query_bad():
	# query switch out of range
	s2 = Switch(client, server_callsign, loop, 999)
	def h(tgt, res):
		print("########", tgt, res)
		assert(tgt == 999)
		assert(res is None)
	s2.on_result(h)
loop.schedule(switch_query_bad, 1.0)

def errors():
	client.sendpkt("")
	# reuse challenge of spent transaction
	client.send("%s:SW,MOO A,%s\r" % (server_callsign, s.challenge))
	# challenge too short
	client.send("%s:SW A,%s\r" % (server_callsign, "1111111"))
	# challenge too short
	client.sendpkt("%s:SW C,%s" % (server_callsign, "1111111"))
	# invalid type
	client.send("%s:SW B,%s\r" % (server_callsign, "12111111"))
	# invalid type
	client.sendpkt("%s:SW AA,%s" % (server_callsign, "12111111"))
	# invalid number of fields
	client.send("%s:SW A,%s,%s\r" % (server_callsign, "12111111", "12345678"))
	# initially valid request that will fail later
	client.sendpkt("%s:SW A,%s" % (server_callsign, "12345678"))
loop.schedule(errors, 30.0)

def errors2():
	# repeat request that will fail later
	client.sendpkt("%s:SW A,%s" % (server_callsign, "12345678"))
loop.schedule(errors2, 35.0)

def errors3():
	# fail request in C packet (wrong response)
	client.sendpkt("%s:SW C,%s,%s,%d,%d" % (server_callsign, "12345678", "12345678", 1, 1))
	# send C packet with bad challenge
	client.sendpkt("%s:SW C,%s,%s,%d,%d" % (server_callsign, "87654321", "12345678", 1, 1))
	# send C packet with short challenge
	client.sendpkt("%s:SW C,%s,%s,%d,%d" % (server_callsign, "87654321", "1234", 1, 1))
	# send C packet w/o target
	client.sendpkt("%s:SW C,%s,%s" % (server_callsign, "87654321", "12345678"))
	# send C packet with bad target
	client.sendpkt("%s:SW C,%s,%s,%d,%d" % (server_callsign, "87654321", "12345678", 0, 1))
	# send C packet with empty target, good value
	client.sendpkt("%s:SW C,%s,%s,,%d" % (server_callsign, "87654321", "12345678", 0))
	# send C packet with good target, empty value
	client.sendpkt("%s:SW C,%s,%s,%d,,bla" % (server_callsign, "87654321", "12345678", 1))
	# send C packet with good target, good value, extra field
	client.sendpkt("%s:SW C,%s,%s,%d,%d,bla" % (server_callsign, "87654321", "12345678", 1, 1))
	# send C packet with bad value 
	client.sendpkt("%s:SW C,%s,%s,%d,%d" % (server_callsign, "87654321", "12345678", 1, 10000000))
loop.schedule(errors3, 40.0)

while loop.service(client, server):
	pass
