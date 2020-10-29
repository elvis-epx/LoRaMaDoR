#!/usr/bin/env python3

import socket, sys, select, time

client_port = int(sys.argv[1])
server_port = int(sys.argv[2])

task_counter = 1
tasks = {}

def schedule(task, to):
	global task_counter, tasks
	tasks[task_counter] = (task, time.time() + to)
	task_counter += 1

def handle(*conns):
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
				print("No EOL yet %d" % self.port)
				break
			print("%d line: %s" % (self.port, line))
	
client = Connection(client_port)
server = Connection(server_port)

client.send("\r!tnc\r")
server.send("\r!tnc\r")

def rst():
	client.send("\r!reset\r")
	server.send("\r!reset\r")

schedule(rst, 60.0)

while handle(client, server):
	# print("handled")
	pass
