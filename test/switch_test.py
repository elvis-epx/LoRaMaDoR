#!/usr/bin/env python3

import sys, select, time, random
from lora_loop import EventLoop
from lora_conn import Connection
from lora_switch import Switch

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

def switch_on():
	s = Switch(client, server_callsign, loop, 1, 1)
	def h(tgt, res):
		print("########", tgt, res)
		assert(res == 1)
		assert(tgt == 1)
	s.on_result(h)
loop.schedule(switch_on, 1.0)

def switch_query():
	s = Switch(client, server_callsign, loop, 2)
	def h(tgt, res):
		print("########", tgt, res)
		assert(res == 0)
		assert(tgt == 2)
	s.on_result(h)
loop.schedule(switch_query, 1.0)

def switch_query_bad():
	# query switch out of range
	s = Switch(client, server_callsign, loop, 999)
	def h(tgt, res):
		print("########", tgt, res)
		assert(tgt == 999)
		assert(res is None)
	s.on_result(h)
loop.schedule(switch_query_bad, 1.0)

def errors():
	# challenge too short
	client.send("%s:SW A,%s\r" % (server_callsign, "1111111"))
	# challenge too short
	client.send("%s:SW C,%s\r" % (server_callsign, "1111111"))
	# invalid type
	client.send("%s:SW B,%s\r" % (server_callsign, "12111111"))
	# invalid type
	client.send("%s:SW AA,%s\r" % (server_callsign, "12111111"))
	# invalid number of fields
	client.send("%s:SW A,%s,%s\r" % (server_callsign, "12111111", "12345678"))
	# initially valid request that will fail later
	client.send("%s:SW A,%s\r" % (server_callsign, "12345678"))
loop.schedule(errors, 30.0)

def errors2():
	# repeat request that will fail later
	client.send("%s:SW A,%s\r" % (server_callsign, "12345678"))
loop.schedule(errors2, 35.0)

def errors3():
	# fail request in C packet (wrong response)
	client.send("%s:SW C,%s,%s,%d,%d\r" % (server_callsign, "12345678", "12345678", 1, 1))
	# send C packet with bad challenge
	client.send("%s:SW C,%s,%s,%d,%d\r" % (server_callsign, "87654321", "12345678", 1, 1))
loop.schedule(errors2, 40.0)

while loop.service(client, server):
	pass
