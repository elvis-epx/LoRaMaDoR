#!/usr/bin/env python3

import hmac
import hashlib

key = 'abracadabra'.encode('utf-8')

to = 'BBBB'
fro = 'AAAA'
ident = '23'
msg = 'Ola'
data = to + fro + ident + msg
data = data.encode('utf-8')

signature = hmac.new(key, msg=data, digestmod=hashlib.sha256).hexdigest()
print(data)
print(signature)
