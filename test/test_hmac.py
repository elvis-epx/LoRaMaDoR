#!/usr/bin/env python3

import hmac
import hashlib

text_key = 'abracadabra'.encode('ascii')
print("Text key:", text_key)
key = hashlib.sha256(bytearray([1]) + text_key).hexdigest()[0:32].encode('ascii')
print("Hashed armored key:", key)

to = 'BBBB'
fro = 'AAAA'
ident = '23'
msg = 'Ola'
data = to + fro + ident + msg
data = data.encode('utf-8')

signature = hmac.new(key, msg=data, digestmod=hashlib.sha256).hexdigest()
print(data)
print(signature)
