# The data handler and UI for the receiver
# useful: http://aprs.gids.nl/nmea/#rmc
# used: https://github.com/Knio/pynmea2
# https://github.com/openlayers/openlayers
# https://www.makeuseof.com/tag/python-javascript-communicate-json/
# https://pythonise.com/series/learning-flask/flask-and-fetch-api

import serial, time, pynmea2
import serial.tools.list_ports
from flask import Flask,render_template,request,make_response,jsonify

# allows flask and ardunio handling to be handled at the same time
import threading




app = Flask("server")

gpsData = {
	"points": {

	}
}

@app.route('/')
def render_map():
	return render_template("map.html")

# Post request endpoint for updated GPS coords
@app.route('/request_update', methods=["POST"])
def fetchGpsData():
	response = make_response(jsonify(gpsData), 200)	# 200=status code
	response.headers["Content-Type"] = "application/json"
	return(response)


def runapp():
	# disabling use_reloader is important as it expects to be running in the main thread: https://stackoverflow.com/questions/31264826/start-a-flask-application-in-separate-thread
	app.run(debug = True, use_reloader = False)

def hardwareInterface():
	cycles = 0
	while True:
		try:
			line = ardunioConnection.readline()
			if(line.decode()!=""):
				# Find the GPGGA line, which gives the needed information
				if(line.decode()[0:6]=="$GPGGA"):

					#if(cycles%2== 0):
						parsedGps = pynmea2.parse(line.decode())

						longitude = parsedGps.longitude
						latitude = parsedGps.latitude
						altitude = parsedGps.altitude

						#if(longitude!=0.0):
						gpsData["points"].update({
"point1": 
	{
		"longitude": longitude,
		"latitude": latitude,
		"altitude": altitude
	}
					})
						print(gpsData)

					#cycles+=1
			


					
		except:
			pass

if __name__ == '__main__':

	# Connection to ardunio, automatically find respective port containing ardunio


	found = False
	ports = list(serial.tools.list_ports.comports())
	for p in ports:
		if "Arduino" in p.description:
			# thanks stackoverflow
			ardunioConnection = serial.Serial(port=p.device, baudrate=9600, timeout=.1)

			print("Ardunio found")
			found = True

	if(not found):
		print("cant find arduino")
		exit()

	t1 = threading.Thread(target=runapp)
	t2 = threading.Thread(target=hardwareInterface)

	t1.start()
	t2.start()

	t1.join()
	t2.join()