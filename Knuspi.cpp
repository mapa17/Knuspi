#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "rs232.h"

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
	double tDiff[N];
	double freq;
	unsigned char b[100];	
	memset(b, 0, 6);
	int nBytes;
	
	LARGE_INTEGER li;
	if(!QueryPerformanceFrequency(&li)){
		fprintf(stderr, "QueryPerformanceFrequency failed!\n");
	} else {			
		freq = double(li.QuadPart)/1000.0; //Scale to ms granularity		
		printf("Timer running at %g hz , div %g\n", (double)(li.QuadPart), freq);
	}
	
			
	//Consume all buffered data - aka flush
	do{
		nBytes = RS232_PollComport( portId, b, 20);
	} while( nBytes );
	memset(b, 0, 100);
	
	printf("Entering main busy loop, hit Ctrl-C to stop program ...\n");
	
	while(1)
	{
		for( int i = 0; i < N ; i++ )
		{			
			Sleep( sleep_time );
            for(int j=0; j < nChars2Send; j++){
                RS232_SendByte( portId, 'a'+i );
            }
				
			if( !QueryPerformanceCounter( &li ) ){
				fprintf(stderr, "Quering the Performance Counter Fails!");
			} else {
				out = li.QuadPart;
				out = out / freq;
				//out = (double) ( (double)(li.QuadPart) / freq );
			}
					
			int it = 0;					
			do{
				nBytes = RS232_PollComport( portId, b, nChars2Send);
				it++;
				if(it > 10000){
					printf("Break\n");
					break;		
				}					
			}while( !nBytes );
			
			if( nBytes != nChars2Send ) {
				printf("Wrong number of received bytes! %d\n", nBytes);
			} else {
				if( !QueryPerformanceCounter( &li ) ){
					fprintf(stderr, "Quering the Performance Counter Fails!");
				} else {
					in = (double) ( (double)(li.QuadPart) / freq );
					in = li.QuadPart;
					in = in / freq;
					tDiff[i] =  in - out;
					//printf("diff %g , in %g , out %g\n", tDiff[i] , in , out);			
				}
				if( b[0] != ('a'+i+1) ){
					printf("Strange Data! %s , expect [%c]\n", b, 'a'+i+1);
				}
			}						
		}
		
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
	exit(0);
}
	