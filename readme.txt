	Laser Clock - displays the wall clock time on a laser projector using the Helios Laser DAC USB to ILDA interface.  
	Digits emulate a 6 digit 7-segment display.

	Written by: Joel Rosenzweig, joel@helitronix.com
	Last Updated: January 28, 2017
	
	Build instructions:
	g++ -L. -Wall -o laserclock laserclock.cpp -lHeliosDacAPI

	Example program execution:
	sudo ./laserclock -size 350 -xpos 0 -ypos 2000 -color 1
	
	Notes:  I used a galvanometer rated for 30K points per second.  At 30K, the projector can
	display approximately 1000 points per vector_list, at 30 vector_lists per second.  This vector_listrate looks reasonable
	and doesn't flicker.

	The rendering algorithm is an experiment.  It works ok, but isn't necessarily optimal.  
	If you make an enhancement, please share it.

	The Helios Laser DAC is available here:
	http://pages.bitlasers.com/helios/

	For the Helios Laser DAC driver and sample code:
	https://github.com/Grix/helios_dac

	Building the Helios driver for Raspberry Pi:
	g++ -Wall -fPIC -O2 -c HeliosDacClass.cpp
	g++ -Wall -fPIC -O2 -c HeliosDac.cpp
	g++ -Wall -fPIC -O2 -c HeliosDacAPI.cpp
	g++ -shared -o libHeliosDacAPI.so HeliosDacAPI.o HeliosDac.o HeliosDacClass.o -L/usr/lib/arm-linux-gnueabihf/ -lusb-1.0

	Helios header files have been modified to fix build issue under Raspberry Pi.  They retain the original license.
