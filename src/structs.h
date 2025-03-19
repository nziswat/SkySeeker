#pragma once
# include <string>

struct aircraftPacket {
	unsigned short int heading;
	unsigned int altitude;
	unsigned int tailNumber;
	std::string flightNumber;
	float lat;
	float longi;
	unsigned short int speed;
	
};