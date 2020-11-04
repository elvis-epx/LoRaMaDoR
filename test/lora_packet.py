#!/usr/bin/env python3

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
			uvalue = None
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

		try:
			ukey = key.decode('utf-8')
		except UnicodeDecodeError:
			self.err = "Unicode error in key"
			return False

		if value is not None:
			for n in range(0, len(value)):
				c = chr(value[n])
				if "= ,:<".find(c) >= 0 or ord(c) >= 127:
					self.err = "Param value with invalid char"
					return False
			try:
				uvalue = value.decode('utf-8')
			except UnicodeDecodeError:
				self.err = "Unicode error in value"
				return False

		self.params[ukey] = uvalue

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
			self.msg = data[i+1:]
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
