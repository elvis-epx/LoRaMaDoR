# this is an attempt to make something useful for the LoRaMaDor project
# this program will check the times (UTC) of the Sun rise, and Sun set for
# a given latitude and longitude. After getting the information it will send
# to a LoRaMaDor node of your choice.
# You need a LoRaMaDor board hooked to the computer.
# Problem, this sends too much for the node to display correctly, but you can read it from a serial port
# copyright 2020, Aug 17, 2020 LeRoy Miller KD8BXP v0.0.1

#python 3

import urllib.request, json 
import serial

ENCODING = 'utf-8'  # don't change me

#change the lat, lng to your lat/lng
with urllib.request.urlopen("https://api.sunrise-sunset.org/json?lat=39.515057&lng=-84.398277&formatted=0") as url:
    data = json.loads(url.read().decode())
    #print(data)
    sunrise = data['results']['sunrise'].replace("T"," ")
    sunset = data['results']['sunset'].replace("T"," ")
    print(sunrise)
    print(sunset)

#change to the node you want this information sent too
msg = '{} {} {} {}'.format('N0CALL-2 Sunrise ',sunrise.replace("+00:00",""), 'Sunset ', sunset.replace("+00:00","")).encode(ENCODING)

print(msg)

print ("Sending page...")
ser = serial.Serial('/dev/ttyUSB0', 115200) #you may need to change the serial port
ser.write(msg)
ser.write(b'\n\r')
ser.close()

# I am pretty new to python, so there is still a lot I need to learn, please feel free to update this with improvements
# but please share so I can see and learn something. Thanks, LeRoy KD8BXP

