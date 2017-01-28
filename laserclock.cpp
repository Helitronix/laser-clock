/*
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

*/

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define MAX_POINTS 10000
#define MAX_PTS_FRAME 1000
#define POINTS_PER_SECOND 30000

HeliosDacClass::HeliosPoint vector_list[MAX_POINTS];
int num_points = 0;
int x_start = 0;
int y_start = 0;

// The left most x coordinate to start drawing the image.
int xpos = 0;

// The bottom most y coordination to start drawing the image.
int ypos = 2000;

// Digit size.  Values of 100 through 350 work well.  Experiment and see.  
// Digits too large will be clipped in size (i.e) when total drawing exceeds 4095 in x or y.  Reduce size if that occurs.
int size = 250;

// Default digit color is RED.
int color = 1;
		
// Vectors are divided into segments so that:
// 	- a constant brightness is achieved across all vectors by having a uniform drawing time per unit length.
//	- non-linearities in X/Y galvo response are minimized.  Vectors are straighter when drawn in shorter segments.
// Chosen suitibly large to avoid flicker.  
// If too low, there are too many points to display.
// If too high, the image will flicker because there are too many points to display per frame, so the frame rate drops.
// If you adjust the size of the display, you may need to alter this divider accordingly.
float divider = 50.0; 

// dwell used for visible lines
// Experimental value
int dwell = 10; 

// dwell used for hidden lines
// Experimental value
int hidden_dwell = 15; 

int DrawPoint(int x, int y, int color)
{
	unsigned short r, g, b;

	if (num_points >= MAX_POINTS)
		return 0;

	// Perform clipping.  Reduce the digit size if clipping occurs.

	if (x > 4095) {
		fprintf(stderr, "Clipping!  Reduce size and/or adjust x position..\n");
		x = 4095;
	}

	if (x < 0) x = 0;

	if (y > 4095) {
		fprintf(stderr, "Clipping!  Reduce size and/or adjust y position..\n");
		y = 4095;
	}

	if (y < 0) y = 0;

	switch (color) {
		case 0:
			r = 0;
			g = 0;
			b = 0;
			break;
		case 1:
			r = 255;
			g = 0;
			b = 0;
			break;
		case 2:
			r = 0;
			g = 255;
			b = 0;
			break;
		case 3:
			r = 0;
			g = 0;
			b = 255;
			break;
		case 4:
			r = 255;
			g = 255;
			b = 0;
			break;
		case 5:
			r = 255;
			g = 0;
			b = 255;
			break;
		case 6:
			r = 0;
			g = 255;
			b = 255;
			break;
		case 7:
			r = 255;
			g = 255;
			b = 255;
			break;
		default:
			r = 255;
			g = 255;
			b = 255;
			break;	
	}

	vector_list[num_points].x = x;
	vector_list[num_points].y = y;
	vector_list[num_points].r = r;
	vector_list[num_points].g = g;
	vector_list[num_points].b = b;
	vector_list[num_points].i = 0xFF;

	return num_points ++;
}


int DrawLineTo(int x, int y, int color)
{

	int i;
	int status = 0;
	float x_length = x - x_start;
	float y_length = y - y_start;
	float vector_length = sqrtf((x_length * x_length) + (y_length * y_length));

	// Interpolate from the previous point (x_start, y_start) to the new point (x,y).
	int num_segments = ceilf(vector_length/divider);
	
	if (color > 0) {
		for (i=1; i <= num_segments; i++) {
			status = DrawPoint(x_start + (x_length/(float)num_segments) * i, 
					y_start + (y_length/(float)num_segments) * i, 
					color);
		}
	} 

	// dwell at the end point, one duration for hidden vectors, 
	// another duration for visible vectors
	if (color == 0) {
		for (i = 0; i < hidden_dwell; i++)
			status = DrawPoint(x,y,color);
	} else {
		for (i = 0; i < dwell; i++)
			status = DrawPoint(x,y,color);
	}


	x_start = x;
	y_start = y;
	
	return status;
}

void DrawZero(int x, int y, int color, int size)
{
	DrawLineTo(x, y, 0);
	DrawLineTo(x + size, y, color);
	DrawLineTo(x + size, y - 2* size, color);
	DrawLineTo(x, y - 2*size, color);
	DrawLineTo(x, y, color);
}
void DrawOne(int x, int y, int color, int size)
{
	DrawLineTo(x + size, y, 0); // An added dwell here helps sharpen the digit.
	DrawLineTo(x + size, y, 0);
	DrawLineTo(x + size, y - 2 * size, color);
}
void DrawTwo(int x, int y, int color, int size)
{

	DrawLineTo(x, y, 0);
	DrawLineTo(x +size, y, color);
	DrawLineTo(x +size, y- size, color);
	DrawLineTo(x, y - size, color);
	DrawLineTo(x, y - 2*size, color);
	DrawLineTo(x+size, y - 2*size, color);

}
void DrawThree(int x, int y, int color, int size)
{
	DrawLineTo(x, y, 0);
	DrawLineTo(x + size, y, color);
	DrawLineTo(x + size, y - 2* size, color);
	DrawLineTo(x, y - 2*size, color);
	DrawLineTo(x, y - size, 0);
	DrawLineTo(x+size, y - size, color);

}
void DrawFour(int x, int y, int color, int size)
{
	DrawLineTo(x, y, 0);
	DrawLineTo(x, y - size, color);
	DrawLineTo(x + size, y - size, color);
	DrawLineTo(x + size, y, 0);
	DrawLineTo(x + size, y - 2 * size, color);

}
void DrawFive(int x, int y, int color, int size)
{
	DrawLineTo(x+size, y, 0);
	DrawLineTo(x, y, color);
	DrawLineTo(x, y - size, color);
	DrawLineTo(x + size, y - size, color);
	DrawLineTo(x + size, y - 2*size, color);
	DrawLineTo(x, y - 2*size, color);

}
void DrawSix(int x, int y, int color, int size)
{
	DrawLineTo(x+size, y, 0);
	DrawLineTo(x, y, color);
	DrawLineTo(x, y - 2 * size, color);
	DrawLineTo(x + size, y - 2 * size, color);
	DrawLineTo(x + size, y - size, color);
	DrawLineTo(x, y - size, color);
}
void DrawSeven(int x, int y, int color, int size)
{
	DrawLineTo(x, y, 0);
	DrawLineTo(x + size, y, color);
	DrawLineTo(x + size, y - 2 * size, color);
}
void DrawEight(int x, int y, int color, int size)
{
	DrawLineTo(x, y, 0);
	DrawLineTo(x + size, y, color);
	DrawLineTo(x + size, y - 2 * size, color);
	DrawLineTo(x, y - 2 * size, color);
	DrawLineTo(x, y, color);
	DrawLineTo(x, y-size, 0);
	DrawLineTo(x+size, y- size, color);
}
void DrawNine(int x, int y, int color, int size)
{
	DrawLineTo(x + size, y-2*size, 0);
	DrawLineTo(x + size, y, color);
	DrawLineTo(x,  y, color);
	DrawLineTo(x, y - size, color);
	DrawLineTo(x+size, y-size, color);
}

int DrawDigit(int n, int x, int y, int color, int size)
{

	switch (n) {
		case 0: DrawZero(x, y, color, size); break;
		case 1: DrawOne(x, y, color, size); break;
		case 2: DrawTwo(x, y, color, size); break;
		case 3: DrawThree(x, y, color, size); break;
		case 4: DrawFour(x, y, color, size); break;
		case 5: DrawFive(x, y, color, size); break;
		case 6: DrawSix(x, y, color, size); break;
		case 7: DrawSeven(x, y, color, size); break;
		case 8: DrawEight(x, y, color, size); break;
		case 9: DrawNine(x, y, color, size); break;
		default: break;
	}
	return 0;
}



int DrawSquare(int x, int y, int color, int size)
{
	int offset = size/2;

	DrawLineTo(x - offset, y - offset, 0);
	DrawLineTo(x - offset +size, y - offset, color);
	DrawLineTo(x - offset +size, y - offset +size, color);
	DrawLineTo(x - offset, y - offset +size, color);
	DrawLineTo(x - offset, y - offset, color);

	return 0;
}

int DrawCircle(int x, int y, int color, float radius, float stepsize)
{
	
	DrawLineTo(x+radius, y, 0);
	for (float theta = 0.0 - (0.5 * stepsize); theta < (360.0 + (0.5 * stepsize)); theta += stepsize) {
		float xf = radius * cos(theta * 3.1415926 / 180.0);
		float yf = radius * sin(theta * 3.1415926 / 180.0);
		DrawLineTo((int)xf + x, (int)yf + y, color);
	}

	return 0;
}

int main(int argc, char ** argv)
{

	for (int i = 1;i < argc;i++)
	{
		if (strcasecmp(argv[i],"-size") == 0)
			size = atoi(argv[i+1]);
		if (strcasecmp(argv[i],"-dwell") == 0)
			dwell = atoi(argv[i+1]);
		if (strcasecmp(argv[i],"-xpos") == 0)
			xpos = atoi(argv[i+1]);
		if (strcasecmp(argv[i],"-ypos") == 0)
			ypos = atoi(argv[i+1]);
		if (strcasecmp(argv[i],"-color") == 0)
			color = atoi(argv[i+1]);
		if (strcasecmp(argv[i],"-hidden_dwell") == 0)
			hidden_dwell = atoi(argv[i+1]);
		if (strcasecmp(argv[i],"-divider") == 0)
			divider = atof(argv[i+1]);
	}

	//connect to DACs and output vector_lists
	HeliosDacClass helios;
	int numDevs = helios.OpenDevices();

	if (numDevs < 1) {
		fprintf(stderr, "No Helios DAC found .. \n");
		exit(0);
	}

	while(1) {
  		
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		num_points = MAX_POINTS;
		num_points = 0;
		
		DrawDigit(tm.tm_hour/10, xpos, ypos, color, size);
		DrawDigit(tm.tm_hour%10, xpos + 2*size, ypos, color, size);
		
		DrawDigit(tm.tm_min/10, xpos + 4*size, ypos, color, size);
		DrawDigit(tm.tm_min%10, xpos + 6*size, ypos, color, size);

		DrawDigit(tm.tm_sec/10, xpos + 8*size, ypos, color, size);
		DrawDigit(tm.tm_sec%10, xpos + 10*size, ypos, color, size);
	
	
		DrawSquare(xpos + (int)(3.5 * size), ypos - size/2, color, size/10);
		DrawSquare(xpos + (int)(3.5 * size), (ypos - size/2) - size, color, size/10);	
		DrawSquare(xpos + (int)(7.5 * size), ypos - size/2, color, size/10);
		DrawSquare(xpos + (int)(7.5 * size), (ypos - size/2) - size, color, size/10);	
	

		fprintf(stderr, "now: %d-%d-%d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		int seconds = tm.tm_sec;
		while(tm.tm_sec == seconds)
		{
			t = time(NULL);
			tm = *localtime(&t);
			while(helios.GetStatus(0) == 0);
			helios.WriteFrame(0, POINTS_PER_SECOND, 0, vector_list, num_points);
		}
	}
}
