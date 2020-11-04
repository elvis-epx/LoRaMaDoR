#!/usr/bin/env python3

import socket
from lora_packet import RxPacket

class Connection:
	def __init__(self, port):
		self.port = port
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(("localhost", port))
		self.readbuf = b''
		self.writebuf = b''
		self.EOL = b'\r\n'
		self.handlers = {
			"debug: ": self.interpret_debug,
			"cli: ": self.interpret_cli,
			"net: ": self.interpret_net,
			"callsign: ": self.interpret_callsign,
			"pkt: ": self.interpret_packet,
			"!tnc": self.interpret_tnc,
		}
		self.proto_handlers = {}

	def send(self, data):
		self.writebuf += data.encode("utf-8")

	def recv(self):
		data, self.readbuf = self.readbuf, ""
		return data

	def do_recv(self):
		# print("reading %d..." % self.port)
		data = self.sock.recv(1500)
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
		print("%d debug %s" % (self.port, line.decode('utf-8')))
		return True

	def interpret_cli(self, line):
		print("%d cli %s" % (self.port, line.decode('utf-8')))
		return True

	def interpret_tnc(self, line):
		# Echo of "!tnc" command we send via serial
		return True

	def interpret_net(self, line):
		print("%d net %s" % (self.port, line.decode('utf-8')))
		return True
	
	def interpret_callsign(self, line):
		print("%d callsign %s" % (self.port, line.decode('utf-8')))
		return True
	
	def interpret_packet(self, line):
		print("%d packet %s" % (self.port, line))
		i = line.find(b' ')
		if i < 0:
			print("%d invalid packet header, no RSSI delim" % self.port)
			return True
		try:
			rssi = float(line[:i])
		except ValueError:
			print("%d invalid RSSI value" % self.port)
			return True
		line = line[i+1:]

		i = line.find(b' ')
		if i < 0:
			print("%d invalid packet header, no size delim" % self.port)
			return True
		if i >= 3:
			print("%d invalid packet header, delim size too big" % self.port)
			return True
		try:
			size = int(line[:i], 10)
		except ValueError:
			print("%d invalid packet delim" % self.port)
			return True
		line = line[i+1:]
		if len(line) > size:
			print("%d packet bigger than expected" % self.port)
			return True
		needs = size - len(line)
		if needs > 0:
			# packet may contain EOL in payload; fetch the rest
			if len(self.readbuf) < needs:
				print("%d packet not complete yet, returning to buffer" % self.port)
				return False
			line += self.readbuf[:needs]
			self.readbuf = self.readbuf[needs:]

		print("%d unwrapped packet RSSI=%f %s" % (self.port, rssi, line))
		p = RxPacket(line)
		if p.err:
			print("\tinvalid packet: %s" % p.err)
			return True
		else:
			print("%d parsed packet %s" % (self.port, str(p)))

		for key, value in p.params.items():
			if key in self.proto_handlers:
				print("Calling handler for %s" % key)
				self.proto_handlers[key].rx(p)
				break

		return True
