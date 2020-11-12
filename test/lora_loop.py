#!/usr/bin/env python3

import socket, sys, select, time, random

class EventLoop:
	def __init__(self):
		self.task_counter = 1
		self.tasks = {}

	def schedule(self, task, to):
		handle = self.task_counter
		self.task_counter += 1
		self.tasks[handle] = (task, time.time() + to)
		return handle

	def cancel(self, handle):
		del self.tasks[handle]

	def service(self, *conns):
		rd = []
		wr = []
		for conn in conns:
			if not conn.sock or conn.sock.fileno() < 0:
				continue
			rd.append(conn.sock)
			if conn.writebuf:
				wr.append(conn.sock)
		if not rd and not wr:
			return False
	
		min_to = time.time() + 60
		for n, task_record in self.tasks.items():
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
		to_call = []
		for n, task_record in self.tasks.items():
			task, to = task_record
			if to < now:
				to_call.append(task)
				done.append(n)
		for task in to_call:
			task()
		for n in done:
			del self.tasks[n]
		return True
