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

#define MAX_BUFFER_SIZE 100
int main( int argc, char** argv)
{
	int portId;
	char portName[20];
	int sleep_time;
	int nChars2Send;
	int arduinoTextLength;
	int nTotalRead;
	
	if( argc < 5)
	{
		//fprintf(stderr, "Missing Serial Port Number as Program argument!\n");
		fprintf(stderr, "Usage %s PORT_ID TEST_STRING_LENGTH SLEEP_IN_MS ARDUINO_TEXT_LENGTH\n", argv[0]);
		exit(1);
	} else {
		portId = atoi( argv[1] );
		nChars2Send = atoi( argv[2] );
		sleep_time = atoi( argv[3] );
		arduinoTextLength = atoi( argv[4] );
		
		if( nChars2Send >= MAX_BUFFER_SIZE )
		{
			fprintf(stderr, "TEST_STRING_LEGNTH too long. Max %d\n", MAX_BUFFER_SIZE);
			exit(1);
		}
		
		if( sleep_time <= 10 )
		{
			fprintf(stderr, "SLEEP_IN_MS too low! Min %g\n", 10.0 );
			exit(1);
		}
	}
	
	RS232_comportIdx2Name( portId, portName );
	printf("Using RS232 with Port %s\n", portName );
	
	if( RS232_OpenComport( portId, 115200 ) )
	{
		fprintf(stderr, "Opening Serial Port failed!");
		exit(2);
	}
			
	#define N 10
	double out;
	double in;
	double now;
	double tDiff[N];
	int tCnter = -1;
	unsigned char b[MAX_BUFFER_SIZE];	
	int bIdx;
	int nBytes;
	unsigned char START_SYMBOL;
	
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
	
	bIdx = 0;
	out = getTime();
	START_SYMBOL = '@';
	while(1)
	{
#ifdef __linux__
		usleep( 1000 );
#else				
		Sleep( 1 );
#endif
		nBytes = RS232_PollComport( portId, &(b[bIdx]), nChars2Send);		
		bIdx += nBytes;
		nTotalRead += nBytes;
				
		//Periodically Send messages
		now = getTime();
		if( (now - out) > sleep_time )
		{					
			//printf("S ... \n");
			for(int j=0; j < nChars2Send; j++){
				RS232_SendByte( portId, START_SYMBOL+j );
			}					
			//printf("S2 ... \n");			
			out = getTime();
		}
						
		//Detect if this response is an echo
		if( (b[0] == START_SYMBOL+1) && ( bIdx >= nChars2Send ) )
		{			
			//printf("R ... \n");
			in = getTime();
			tCnter++;
			tDiff[tCnter] =  in - out;
			
			// Shift data
			memcpy( b, &(b[nChars2Send]), bIdx - nChars2Send );
			bIdx -= nChars2Send;
		} else if ( bIdx >= nChars2Send ) {
			//printf("Try to detect frame start ...");
			int j;
			for( j = 0; j < bIdx; j++ )
			{
				if( b[j] == (START_SYMBOL+1) )
					break;
			}
			memcpy( b, &(b[j]), bIdx - j );
			bIdx -= j;
			//printf("Shifted by %d , bIdx %d\n", j, bIdx);
		}
		
		if( tCnter >= (N-1) )
		{
			tCnter = -1;
			double totalTime = 0;
			double td;
			for( int i = 0; i < N; i++ )
			{				
				if( tDiff[i] < 0 ){
					printf("Strange strange ...");
				} else {
					totalTime += tDiff[i];
				}
			}
			double avgDiff =  (double) (totalTime / (double) N);
			printf("RTT %g , N = %d , Send %d, Sleep %d ms, Recv %d, Recv Total %d\n", avgDiff, N , nChars2Send, sleep_time, arduinoTextLength, nTotalRead);
			nTotalRead = 0;
			
			RS232_SendByte( portId, arduinoTextLength );
		}					
	}
	exit(0);
}
	
