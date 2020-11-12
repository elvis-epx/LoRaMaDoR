#!/usr/bin/env python3

import socket
from lora_packet import RxPacket

class Connection:
	def __init__(self, port):
		self.port = port
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			self.sock.connect(("localhost", port))
		except ConnectionRefusedError:
			self.sock.close()
		self.readbuf = b''
		self.writebuf = b''
		self.EOL = b'\r\n'
		self.handlers = {
			"debug: ": self.interpret_debug,
			"cli: ": self.interpret_cli,
			"net: ": self.interpret_net,
			"callsign: ": self.interpret_callsign,
			"pkrx: ": self.interpret_packet,
			"!tnc": self.interpret_tnc,
		}
		self.protocol_handlers = {}

	def add_proto_handler(self, keys, klass):
		for key in keys:
			self.protocol_handlers[key] = klass

	def send(self, data):
		self.writebuf += data.encode("utf-8")

	def sendpkt(self, data):
		if isinstance(data, str):
			data = data.encode("utf-8")
		self.writebuf += b"!pktx " + data.hex().encode("utf-8") + b"\r"

	def recv(self):
		data, self.readbuf = self.readbuf, ""
		return data

	def do_recv(self):
		# print("reading %d..." % self.port)
		try:
			data = self.sock.recv(1500)
		except ConnectionResetError:
			data = ""
		if len(data) == 0:
			print("Conn %d closed on recv" % self.port)
			self.sock.close()
			self.sock = None
		else:
			self.readbuf += data
			self.interpret()

	def do_send(self):
		# print("writing %d..." % self.port)
		n = self.sock.send(self.writebuf)
		if n < 0:
			print("Conn %d error on send" % self.port)
			self.sock.close()
			self.sock = None
		elif n == 0:
			print("Conn %d closed on send" % self.port)
			self.active = False
			self.sock.close()
			self.sock = None
		else:
			self.writebuf = self.writebuf[n:]

	def readline(self):
		i = self.readbuf.find(self.EOL)
		if i < 0:
			return None
		line, self.readbuf = \
			self.readbuf[:i], self.readbuf[i+len(self.EOL):]
		return line

	def interpret(self):
		while self.readbuf:
			line = self.readline()
			if line is None:
				break
			if not line:
				continue
			# print("%d line: %s" % (self.port, line))
			if not self.interpret_line(line):
				# not complete; put back
				self.readbuf = line + self.EOL + self.readbuf

	def interpret_line(self, line):
		for header, handler in self.handlers.items():
			header = header.encode("utf-8")
			if line.startswith(header):
				return handler(line[len(header):])
		print("Unrecognized line %d: %s" % (self.port, line))
		return True

	def interpret_debug(self, line):
		print("%d debug" % self.port, line)
		return True

	def interpret_cli(self, line):
		print("%d cli" % self.port, line)
		return True

	def interpret_tnc(self, line):
		# Echo of "!tnc" command we send via serial
		return True

	def interpret_net(self, line):
		print("%d net" % self.port, line)
		return True
	
	def interpret_callsign(self, line):
		print("%d callsign" % self.port, line)
		return True
	
	def interpret_packet(self, line):
		print("%d packet" % self.port, line)
		i = line.find(b' ')
		if i < 0:
			print("%d invalid packet header, no RSSI delim" % self.port)
			return True
		try:
			rssi = int(line[:i], 10)
		except ValueError:
			print("%d invalid RSSI value" % self.port)
			return True
		line = line[i+1:]

		print("%d unwrapped packet RSSI=%d %s" % (self.port, rssi, line))
		p = RxPacket(line)
		if p.err:
			print("\tinvalid packet: %s" % p.err)
			return True
		else:
			print("%d parsed packet %s" % (self.port, str(p)))

		for key, value in p.params.items():
			if key in self.protocol_handlers:
				print("Calling handler for %s" % key)
				if self.protocol_handlers[key].rx(p):
					break

		return True
