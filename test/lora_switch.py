#!/usr/bin/env python3

import time, random

class Switch:
	transactions = {}

	def __init__(self, tnc, server, loop, target, value=None):
		self.tnc = tnc
		self.server = server
		self.loop = loop
		self.target = target
		self.value = value 
		self.challenge = self.gen_challenge(8)
		self.sendA()
		Switch.transactions[self.challenge] = self

	def gen_challenge(self, length):
		c = ""
		for i in range(0, length):
			c += "0123456789abcdefghijklmnopqrstuvwxyz"[random.randint(0, 35)]
		return c

	@staticmethod
	def rx(pkt):
		print("SW: received packet", pkt.msg)
		try:
			umsg = pkt.msg.decode('utf-8')
		except UnicodeDecodeError:
			print("SW: packet msg has non-ASCII chars")
			return True

		fields = umsg.split(",")
		if len(fields) < 3:
			print("SW: packet msg has less than 3 fields")
			return True
		ptype, challenge, response = fields[0:3]

		if challenge not in Switch.transactions:
			print("SW: packet msg has unknown challenge")
			return True
			
		if ptype == "B":
			Switch.transactions[challenge].rxB(fields)
		elif ptype == "D":
			Switch.transactions[challenge].rxD(fields)
		else:
			print("SW: packet msg of unknown type")

		return True

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
		if self.value is None:
			# query
			self.tnc.send("%s:SW C,%s,%s,%d,?\r" % \
				(self.server, self.challenge, self.response,
				self.target))
		else:
			# set
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
		del Switch.transactions[self.challenge]
