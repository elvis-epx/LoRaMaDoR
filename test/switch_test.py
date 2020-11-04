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
	Switch(client, server_callsign, loop, 1, 1)
loop.schedule(switch_on, 1.0)

def switch_query():
	Switch(client, server_callsign, loop, 2)
loop.schedule(switch_query, 1.0)

while loop.service(client, server):
	pass
