# The data handler and UI for the receiver
# useful: http://aprs.gids.nl/nmea/#rmc
# used: https://github.com/Knio/pynmea2

import serial, time, pynmea2


ardunioConnection = serial.Serial(port='/dev/ttyACM1', baudrate=9600, timeout=.1)

while True:
	try:
		line = ardunioConnection.readline()
		if(line.decode()!=""):
			# Find the GPGLL line, which gives the location
			if(line.decode()[0:6]=="$GPGLL"):
				gpsData = pynmea2.parse(line.decode())
				print(gpsData.longitude)
	except:
		pass