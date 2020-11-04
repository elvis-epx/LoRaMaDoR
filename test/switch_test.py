#!/usr/bin/env python3

import sys, select, time, random
from lora_loop import EventLoop
from lora_conn import Connection

client_port = int(sys.argv[1])
server_port = int(sys.argv[2])
server_callsign = sys.argv[3]

class Switch:
	def __init__(self, tnc, server, loop, target, value):
		self.tnc = tnc
		self.tnc.proto_handlers["SWC"] = self
		self.server = server
		self.loop = loop
		self.target = target
		self.value = value 
		self.challenge = self.gen_challenge(8)
		self.sendA()

	def gen_challenge(self, length):
		c = ""
		for i in range(0, length):
			c += "0123456789abcdefghijklmnopqrstuvwxyz"[random.randint(0, 35)]
		return c

	def rx(self, pkt):
		print("SW: received packet", pkt.msg)
		try:
			umsg = pkt.msg.decode('utf-8')
		except UnicodeDecodeError:
			print("SW: packet msg has non-ASCII chars")
			return
		fields = umsg.split(",")
		if fields[0] == "B":
			self.rxB(fields)
		elif fields[0] == "D":
			self.rxD(fields)

	def sendA(self):
		print("SW: sending packet A")
		self.tnc.send("%s:SW A,%s\r" % (self.server, self.challenge))
		self.state = 'A'
		self.to = self.loop.schedule(self.timeoutA, 3.0)

	def timeoutA(self):
		# TODO maximum number of requests, exponential backoff
		self.sendA()

	def rxB(self, fields):
		if len(fields) < 3:
			print("SW: received invalid packet B")
			return
		if self.state != 'A':
			print("SW: received packet B but state not A")
			return
		if fields[1] != self.challenge:
			print("SW: received packet B with wrong challenge")
			return
		if len(fields[2]) < 8:
			print("SW: received packet B with short response")
			return
		self.response = fields[2]
		if self.to:
			self.loop.cancel(self.to)
			self.to = None
		self.sendC()

	def sendC(self):
		print("SW: sending packet C")
		self.tnc.send("%s:SW C,%s,%s,%d,%d\r" % \
			(self.server, self.challenge, self.response,
			self.target, self.value))
		self.state = 'C'
		self.to = self.loop.schedule(self.timeoutC, 3.0)

	def timeoutC(self):
		# TODO maximum number of requests, exponential backoff
		self.sendC()

	def rxD(self, fields):
		if len(fields) < 5:
			print("SW: received invalid packet D")
			return
		if self.state != 'C':
			print("SW: received packet D but state not C")
			return
		if fields[1] != self.challenge:
			print("SW: received packet D with wrong challenge")
			return
		if fields[2] != self.response:
			print("SW: received packet D with wrong response")
			return
		if self.to:
			self.loop.cancel(self.to)
			self.to = None
		# TODO Check target and state
		print("SW: **** finished transaction ****")
		self.state = 'E'


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

def switch_on():
	Switch(client, server_callsign, loop, 1, 1)
loop.schedule(switch_on, 1.0)

while loop.service(client, server):
	pass
