#!/usr/bin/env python3

import time, random

class Ping:
	transactions = {}

	def __init__(self, tnc, server, loop, payload):
		self.tnc = tnc
		self.server = server
		self.loop = loop
		self.payload = payload
		self.callback = lambda payload: None
		self.sendPing()
		# TODO better transaction id
		Ping.transactions[self.server] = self

	def on_result(self, cb):
		self.callback = cb

	@staticmethod
	def rx(pkt):
		print("PING: received packet", pkt.msg)
		if pkt.fromm not in Ping.transactions:
			print("PING: pong packet has unknown sender")
			return True
		Ping.transactions[pkt.fromm].rxPong(pkt)
		return True

	def sendPing(self):
		print("PING: sending")
		self.tnc.sendpkt(self.server.encode("ascii") + b":PING " + self.payload)
		self.to = self.loop.schedule(self.timeoutPing, 10.0)

	def timeoutPing(self):
		# TODO maximum number of requests, exponential backoff
		self.sendPing()

	def rxPong(self, pkt):
		if self.to:
			self.loop.cancel(self.to)
		self.to = None
		self.callback(pkt.msg)
		del Ping.transactions[pkt.fromm]
		print("PING: **** finished transaction ****")
