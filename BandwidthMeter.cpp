#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef __linux__
	#include <windows.h>
#endif
#include "rs232.h"

double ClockFrequency;

double getTime()
{
	double t;

#ifdef __linux__
	timespec ts;
	// clock_gettime(CLOCK_MONOTONIC, &ts); // Works on FreeBSD
	clock_gettime(CLOCK_REALTIME, &ts); // Works on Linux
	t = ts.tv_sec * 1000.0 + (ts.tv_nsec / 1000000.0);
#else
	LARGE_INTEGER li;
	
	if( !QueryPerformanceCounter( &li ) ){
		fprintf(stderr, "Quering the Performance Counter Fails!");
	} else {		
		t = li.QuadPart;
		t = t / ClockFrequency;		
	}
#endif
	return t;
}

#define MAX_BUFFER_SIZE 1000
int main( int argc, char** argv)
{
	int portId;
	char portName[20];
	int sleep_time;
	int nTotalRead;
	int nBytes;
	
	if( argc < 1)
	{		
		fprintf(stderr, "Usage %s PORT_ID \n", argv[0]);
		exit(1);
	} else {
		portId = atoi( argv[1] );
	}
	
	RS232_comportIdx2Name( portId, portName );
	printf("Using RS232 with Port %s\n", portName );
	
	if( RS232_OpenComport( portId, 115200 ) )
	{
		fprintf(stderr, "Opening Serial Port failed!");
		exit(2);
	}
			
	#define N 10
	double last;
	double now;
	unsigned char b[MAX_BUFFER_SIZE];		
	
#ifdef __linux__
	printf("Linux...");
#else
	LARGE_INTEGER li;
	if(!QueryPerformanceFrequency(&li)){
		fprintf(stderr, "QueryPerformanceFrequency failed!\n");
	} else {			
		ClockFrequency = double(li.QuadPart)/1000.0; //Scale to ms granularity		
		printf("Timer running at %g hz , div %g\n", (double)(li.QuadPart), ClockFrequency);
	}
#endif

	printf("Entering main busy loop, hit Ctrl-C to stop program ...\n");
	
	nTotalRead = 0;
	last = getTime();
	sleep_time = 1000;
	while(1)
	{
		nBytes = RS232_PollComport( portId, b, MAX_BUFFER_SIZE);				
		nTotalRead += nBytes;
				
		//Periodically Send messages
		now = getTime();
		if( (now - last) > sleep_time )
		{								
			printf("%d Bytes/sec\n", nTotalRead);
			nTotalRead = 0;	
			last = now;
		}							
	}
	exit(0);
}
	
