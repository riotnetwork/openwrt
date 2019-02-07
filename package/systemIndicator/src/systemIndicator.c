#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>



#include "ifconfig.h"

#define BUFFERSIZE    10000
#define TEMPFILENAME   "/tmp/state.dat"
#define TEMPFILENAMEHUMAN   "/tmp/stateHuman.dat"
#define MAX_WLANDATA_RATE  1000000.0
#define NANO_PER_SEC  1000000000.0

//#define VERBOSE_OUTPUT

#define REVISION  "1.0"  // Initial revision

#pragma pack(1)  // byte align

const char *upstring = "TX bytes:";
const char *downstring = "RX bytes:";

typedef union
{
     struct bitfield
     {
         // bitfield begin
         uint8_t ethStatus : 1;
         uint8_t wlanStatus : 1;
         uint8_t error : 1;
         uint8_t unused :5 ;
         // bitfield end
     }bits;
     uint8_t byte;
} statusBitfield;

// data struct as recieved from network controller
typedef struct dataStruct
{
     statusBitfield systemStatus;
     uint8_t upRate;
     uint8_t downRate;
} systemData;



static struct timespec start,end;
static double start_sec,end_sec,elapsed_sec;

void PushResultOut(systemData *result, int length)
{
   // FILE *fid3 = fopen("/dev/stdout","w");
   // if (fid3 != NULL)
  //  {
#ifdef VERBOSE_OUTPUT
        printf("ethstatus: %i; wlanstatus: %i, Error %i, uprate: %x, downrate %x \n",result->systemStatus.bits.ethStatus,result->systemStatus.bits.wlanStatus,result->systemStatus.bits.error,(uint8_t)result->upRate,(uint8_t)result->downRate);
#else
      fwrite(result,1,length,stdout);
//printf("ethstatus: %i; wlanstatus: %i, Error %i, uprate: %x, downrate %x \n",result->systemStatus.bits.ethStatus,result->systemStatus.bits.wlanStatus,result->systemStatus.bits.error,(uint8_t)result->upRate,(uint8_t)result->downRate);
//printf(" 0x%x  0x%x 0x%x \n",result->systemStatus.byte,(uint8_t)result->upRate,(uint8_t)result->downRate);

#endif
      //  fclose(fid3);
   // }
   // else{
	
   // }
}


int main(int argc, char **argv){
    char     buffer[BUFFERSIZE];
    systemData result;
    int    bindex = 0;
    FILE  *instream;
    int   bytes_read=0;
    int   buffer_size=0;
    char  wlanname[100];
    char  ethernetname[100];
    int feednumber = -1;

    double datarateup = 0;
    double dataratedown = 0;
    double dataup = 0;
    double datadown = 0;
    double lastdataup = 0;
    double lastdatadown = 0;


    buffer_size=sizeof(unsigned char)*BUFFERSIZE;


    // interpret according to interfaces on command line - must be 2 arguments
    if (argc < 3){
	 fprintf(stderr, "not enough arguments, need 2 [wlan name] [eth name], got only %d.\n",argc-1);
         return -1;
	}

    	strcpy(wlanname,argv[1]);
    	strcpy(ethernetname,argv[2]);
#ifdef VERBOSE_OUTPUT
 	printf("got %s, %s\n",wlanname,ethernetname);
#endif

    if (argc >= 4){
        feednumber = atoi(argv[3]);
#ifdef VERBOSE_OUTPUT
	printf("got %s, %s ,%d \n",wlanname,ethernetname,feednumber);
#endif
	}


    if (feednumber == -1)
    {
        bindex = 0;
       /* open stdin for reading */
      // instream=fopen("/dev/stdin","r");
#ifdef VERBOSE_OUTPUT
	printf("reading buffer");
#endif
        /* did it open? */
        if(1){
            /* read from stdin until it's end */
            while( (bytes_read=fread(&buffer[bindex], 1, 1, stdin))==1 )
            {
                // insert in rolling buffer
		
#ifdef VERBOSE_OUTPUT
                fprintf(stderr, "%c", buffer[bindex]);
#endif
		bindex++;
            }
#ifdef VERBOSE_OUTPUT
		printf("done reading buffer\n");
#endif
        }
        /* if any error occured, exit with an error message */
        else{
            fprintf(stderr, "ERROR opening stdin. aborting.\n");
            exit(1);
        }
#ifdef VERBOSE_OUTPUT
        printf("Finished reading from pipe.\n");
#endif
    }
    else
    {
        // test cases wher ethe pipe is not used
        switch (feednumber)
        {
        default:
        case (1):
            strcpy(buffer,ifconfig1);
            break;
        case (2):
            strcpy(buffer,ifconfig2);
            break;
        case (3):
            strcpy(buffer,ifconfig3);
            break;
        case (4):
            strcpy(buffer,ifconfig4);
            break;
        }

        bindex = strlen(buffer);
    }
    // parse the text for relevent parameters
    result.systemStatus.bits.error = 0;
    result.downRate = 0;
    result.upRate = 0;
    // find the first interface
    char *first = strstr(buffer,ethernetname);
    if (first == NULL)
    {
        result.systemStatus.bits.ethStatus = 0;
        PushResultOut(&result,sizeof(result));
        return -1;
    }
    else{
        result.systemStatus.bits.ethStatus = 1;
	}
    char *second = strstr(buffer,wlanname);
    if (second == NULL)
    {
        result.systemStatus.bits.wlanStatus = 0;
        PushResultOut(&result,sizeof(result));
        return -1;
    }
    else{
        result.systemStatus.bits.wlanStatus = 1;
	}


    // find out up bytes and down bytes in wlan interface
    char *upbytes = strstr(second,upstring);
    if (upbytes == NULL)
    {
        result.systemStatus.bits.error = 1;
        PushResultOut(&result,sizeof(result));
        return -1;
    }
    else
    {
        dataup = atof(upbytes + strlen(upstring));
    }

    char *downbytes = strstr(second,downstring);
    if (downbytes == NULL)
    {
        result.systemStatus.bits.error = 1;
        PushResultOut(&result,sizeof(result));
        return -1;
    }
    else
    {
        datadown = atof(downbytes + strlen(downstring));
    }

    // get  current time
    clock_gettime(CLOCK_REALTIME, &end);
    end_sec = end.tv_sec + end.tv_nsec/NANO_PER_SEC;
    // get old time
    FILE *fid = fopen(TEMPFILENAME,"rb");
    if (fid == NULL)
    {
        // could not open file - does not exist yet
        datarateup = 0;
        dataratedown = 0;
    }
    else
    {
        fread(&lastdataup,1,sizeof(lastdataup),fid);
        fread(&lastdatadown,1,sizeof(lastdatadown),fid);
        fread(&start,1,sizeof(start),fid);
        fclose(fid);
        start_sec = start.tv_sec + start.tv_nsec/NANO_PER_SEC;
        elapsed_sec = end_sec - start_sec;
        datarateup = (dataup-lastdataup)/elapsed_sec;
        dataratedown = (datadown-lastdatadown)/elapsed_sec;
    }
    FILE *fid2 = fopen(TEMPFILENAME,"wb");
    if (fid2 != NULL)
    {
        fwrite(&dataup,1,sizeof(dataup),fid2);
        fwrite(&datadown,1,sizeof(datadown),fid2);
        fwrite(&end,1,sizeof(end),fid2);
        fclose (fid2);
    }

	uint16_t downrateCalc = ((dataratedown*255.0)/MAX_WLANDATA_RATE);
	uint16_t uprateCalc = ((datarateup*255.0)/MAX_WLANDATA_RATE); 
	if(downrateCalc >254){
	result.downRate = (uint8_t)254;
	}
	else{
	result.downRate = (uint8_t)downrateCalc;
	}
	if(uprateCalc >254){
	result.upRate = (uint8_t)254;
	}
	else{
	result.upRate = (uint8_t)uprateCalc;
	} 

	if(result.upRate<5){
	result.upRate = 5;
	}
	if(result.downRate <5){
	result.downRate = 5;
	}

	// save a human readable file 
 FILE *fid3 = fopen(TEMPFILENAMEHUMAN,"wb");
    if (fid3 != NULL)
    {
 	fprintf(fid3,"Interface %s \n",second);
        fprintf(fid3,"down: %f Bps, %u meteorRate, max %f \n",dataratedown,(uint8_t)result.downRate,(double)MAX_WLANDATA_RATE);
	fprintf(fid3,"up: %f Bps, %u meteorRate, max %f  \n",datarateup,(uint8_t)result.upRate,(double)MAX_WLANDATA_RATE);

        fclose (fid3);
    }
    PushResultOut(&result,sizeof(result));


    return(0);
}
