#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include "rs232.h"

double ClockFrequency;

double getTime()
{
	double t;
	LARGE_INTEGER li;
	
	if( !QueryPerformanceCounter( &li ) ){
		fprintf(stderr, "Quering the Performance Counter Fails!");
	} else {		
		t = li.QuadPart;
		t = t / ClockFrequency;		
	}
	return t;
}

#define MAX_BUFFER_SIZE 100
int main( int argc, char** argv)
{
	int portId;
	char portName[20];
	int sleep_time;
	int nChars2Send;
	
	if( argc < 4)
	{
		//fprintf(stderr, "Missing Serial Port Number as Program argument!\n");
		fprintf(stderr, "Usage %s PORT_ID TEST_STRING_LENGTH SLEEP_IN_MS\n", argv[0]);
		exit(1);
	} else {
		portId = atoi( argv[1] );
		nChars2Send = atoi( argv[2] );
		sleep_time = atoi( argv[3] );
		
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
	double lastTimeMsgSend;	
	
	LARGE_INTEGER li;
	if(!QueryPerformanceFrequency(&li)){
		fprintf(stderr, "QueryPerformanceFrequency failed!\n");
	} else {			
		ClockFrequency = double(li.QuadPart)/1000.0; //Scale to ms granularity		
		printf("Timer running at %g hz , div %g\n", (double)(li.QuadPart), ClockFrequency);
	}
	
			
	//Consume all buffered data - aka flush
	do{
		nBytes = RS232_PollComport( portId, b, 20);
	} while( nBytes );
	memset(b, 0, 100);
	
	printf("Entering main busy loop, hit Ctrl-C to stop program ...\n");
	
	bIdx = 0;
	lastTimeMsgSend = getTime();
	while(1)
	{
				
		Sleep( 1 );
		nBytes = RS232_PollComport( portId, &(b[bIdx]), nChars2Send);		
		bIdx += nBytes;
		
		//if( nBytes ){
		//	printf("R %d , bIdx %d\n", nBytes, bIdx );
		//}
				
		//Periodically Send messages
		now = getTime();
		if( (now - lastTimeMsgSend) > sleep_time )
		{					
			//printf("S ... \n");
			for(int j=0; j < nChars2Send; j++){
				RS232_SendByte( portId, '#'+j );
			}								
			out = getTime();
			lastTimeMsgSend = out;
		}
						
		//Detect if this response is an echo
		if( (b[0] == '#'+1) && ( bIdx >= nChars2Send ) )
		{
			//printf("Found a package ...\n");
			in = getTime();
			tCnter++;
			tDiff[tCnter] =  in - out;
			
			// Shift data
			memcpy( b, &(b[nChars2Send]), bIdx - nChars2Send );
			bIdx -= nChars2Send;
		} else if ( bIdx >= nChars2Send ) {
			printf("Try to detect frame start ...");
			int j;
			for( j = 0; j < bIdx; j++ )
			{
				if( b[j] == ('#'+1) )
					break;
			}
			memcpy( b, &(b[j]), bIdx - j );
			bIdx -= j;
			printf("Shifted by %d\n", j);
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
			printf("RTT %g , N = %d , textLength %d , Sleep %d ms\n", avgDiff, N , nChars2Send, sleep_time);
		}					
	}
	exit(0);
}
	
