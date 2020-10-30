#!/usr/bin/env python3

import socket, sys, select, time

client_port = int(sys.argv[1])
server_port = int(sys.argv[2])

class RxPacket:
	def __init__(self, data):
		self.params = {}
		self.msg = b''
		self.to = ""
		self.fromm = ""
		self.ident = 0
		self.err = None
		self.decode_l3(data)

	def parse_symbol_param(self, data):
		i = data.find(b'=')
		if i < 0:
			# naked key
			key = data
			value = None
		else:
			key = data[:i]
			value = data[i+1:]

		for n in range(0, len(key)):
			c = key[n]
			if c >= ord('0') and c <= ord('9'):
				pass
			elif c >= ord('a') and c <= ord('z'):
				pass
			elif c >= ord('A') and c <= ord('Z'):
				pass
			else:
				self.err = "Param key with invalid char"
				return False

		if value is not None:
			for n in range(0, len(value)):
				c = chr(value[n])
				if "= ,:<".find(c) >= 0 or ord(c) >= 127:
					self.err = "Param value with invalid char"
					return False

		try:
			self.params[key.decode('utf-8').upper()] = value.decode('utf-8')
		except UnicodeDecodeError:
			self.err = "Unicode error in key or value"
			return False

		return True

	def parse_ident_param(self, data):
		try:
			self.ident = int(data, 10)
		except ValueError:
			self.err = "Ident param not a valid decimal number"
			return False
		if self.ident <= 0:
			self.err = "Ident <= 0"
			return False
		if self.ident > 999999:
			self.err = "Ident > 999999"
			return False
		if len(data) != len(str(self.ident)):
			self.err = "Ident int repr with different length"
			return False
		return True

	def parse_param(self, data):
		c = data[0]
		if c >= ord('0') and c <= ord('9'):
			return self.parse_ident_param(data)
		elif c >= ord('a') and c <= ord('z'):
			return self.parse_symbol_param(data)
		elif c >= ord('A') and c <= ord('Z'):
			return self.parse_symbol_param(data)
		else:
			self.err = "Param with invalid first char"
			return False

	def parse_params(self, data):
		while data:
			comma = data.find(b',')
			if comma < 0:
				# last, or only, param
				param = data
				data = ""
			else:
				param = data[:comma]
				data = data[comma+1:]
			if not param:
				self.err = "Empty param"
				return

			if not self.parse_param(param):
				return
	
	def decode_preamble(self, data):
		d1 = data.find(b'<')
		d2 = data.find(b':')
		if d1 < 0 or d2 < 0:
			self.err = "Basic preamble delimiters"
			return
		elif d1 >= d2:
			self.err = "Basic preamble delimiters order"
			return
		try:
			self.to = data[:d1].decode('utf-8')
			self.fromm = data[d1+1:d2].decode('utf-8')
		except UnicodeDecodeError:
			self.err = "Unicode error in callsign"
			return 
		# TODO check callsign internal format
		self.parse_params(data[d2+1:])

	def decode_l3(self, data):
		i = data.find(b' ')
		if i >= 0:
			preamble = data[:i]
			self.msg = data[i+1]
		else:
			# valid packet with no message
			preamble = data
		self.decode_preamble(preamble)

	def __str__(self):
		sparams = []
		for key, value in self.params.items():
			if value is None:
				sparams.append(key)
			else:
				sparams.append(key + "=" + value)
		sparams = ",".join(sparams)
			
		return "to %s from %s params %s msg %s" % \
			(self.to, self.fromm, sparams, self.msg)

task_counter = 1
tasks = {}

def schedule(task, to):
	global task_counter, tasks
	tasks[task_counter] = (task, time.time() + to)
	task_counter += 1

def service_event_loop(*conns):
	global task_counter, tasks
	rd = []
	wr = []
	for conn in conns:
		if not conn.sock:
			continue
		rd.append(conn.sock)
		if conn.writebuf:
			wr.append(conn.sock)
	if not rd and not wr:
		return False

	min_to = time.time() + 60
	for n, task_record in tasks.items():
		task, to = task_record
		if to < min_to:
			min_to = to
	to = min_to - time.time()
	if to < 0:
		to = 0
	# print("Timeout %f" % to)
	
	rdok, wrok, dummy = select.select(rd, wr, [], to)
	for conn in conns:
		if conn.sock in rdok:
			conn.do_recv()
		if conn.sock in wrok:
			conn.do_send()
	now = time.time()
	done = []
	for n, task_record in tasks.items():
		task, to = task_record
		if to < now:
			# print("Executing task %d" % n)
			task()
			done.append(n)
	for n in done:
		del tasks[n]
	return True

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
		}

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
		data = self.sock.recv(1500)
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
		else:
			print("%d parsed packet %s" % (self.port, str(p)))
		return True

client = Connection(client_port)
server = Connection(server_port)

client.send("\r!tnc\r")
server.send("\r!tnc\r")

def rst():
	client.send("\r!reset\r")
	server.send("\r!reset\r")

schedule(rst, 60.0)

while service_event_loop(client, server):
	# print("loop")
	pass
