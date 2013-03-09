/*Blue Tooth Telephone Code
*Author: Dirk Dubois
*Rev: 4.0
*Date: November 22, 2011
*
*
*/

#include <htc.h>
#include <stdlib.h>
#include <string.h>
#include "usart.h"
#include "delay.h"

//Defines clock frequency for outside functions
#define XTAL_FREQ 4MHZ
#define BAUD_RATE 6 //baud rate of 8929Baud
#define DEBUG 1

//Define Ports
#define HOOK RB6 //HOOK == 1 is off the hook
#define END_ROTARY RB7 //END_ROTARY == 1 stationary
#define ROTARY RB4
#define WAVE1 RA2
#define WAVE2 RA3
#define RINGER_HARDWARE RA1
//Delay Times
#define DELAY_TIME 25 //  change back to 25

//Ring Times
#define LONG_RING 40
#define SHORT_RING 10
#define NUMBER_RINGS 2

//String Constants
#define MAX_MESSAGE_SIZE 50
unsigned char message[MAX_MESSAGE_SIZE]; //message buffer in ISR
//unsigned char finalMessage[MAX_MESSAGE_SIZE]; //final message buffer
unsigned char messageIndex = 0; //message index
unsigned char messageComplete = 0; //Flag for message buffer

//Global Variables
unsigned char connected = 0; //Flag to determine if connected
unsigned char i = 0 ; //coutner variable
/*An isr function the process a recieved message over the UART module
Input: void
Output: void */
void interrupt isr(void)
{
	unsigned char c = getch();
	message[messageIndex] = c;
	
	if( (message[messageIndex] == '\n') || (message[messageIndex] == '\r'))
	{
		messageComplete = 1;
	}
	else
	{	
		//Check to make sure buffer hasn't over flowed
		if(messageIndex == (MAX_MESSAGE_SIZE - 1) )
		{
			messageComplete = 1; //biggest junk you will get flag message complete
		}
		else	
		messageIndex++; //increase pointer
	}
	
}

/* A Ring Function that generates 20Hz signal 180* out of phase
Note, the duty cycle on/off should be 2/4 for a regular ring tone
Input: char, ring time
Output: void */
void ring(char j)
{
	char i;
	RINGER_HARDWARE = 1; //turn on ringer
	DelayMs(100); // let fat capacitor charge
	// i = 10 provides a 0.5 second delay
	//Checks if we are still on the hook otherwise end ring cycle

	for(i= 0;( (i<= j) && (HOOK ==0) ); i++)
	{
		//Set start values for respective square waves
		WAVE1 = 0;
		WAVE2 = 1;

			
		DelayMs(DELAY_TIME); //Delay 10 ms, which is T/2 for 22Hz
	
		//Swap values for respective square waves
		WAVE1 = 1;
		WAVE2 = 0;
		
		DelayMs(DELAY_TIME); //Delay 10 ms, which is T/2 for 22Hz
	}

	//Alternate values to avoid short
	WAVE1 = 1;
	WAVE2 = 0;

	RINGER_HARDWARE = 0; //turn off ringer

}
char dialing()
{
	char dialedNumber, counter; 
	
	for(counter = 0; counter < 8; counter++)
		{
			dialedNumber = 0; //Clear the number we are going to send
				
			while(END_ROTARY == 1 && HOOK == 1)
			{
					//Wait for user to start dialing	
			}

			DelayMs(10); //Wait for switch to debounce

			while(END_ROTARY == 0 && HOOK == 1)
			{
				//Count number of toggles
				while(ROTARY = 1)
				{
					if(END_ROTARY == 1 || HOOK == 0 )
					break;
				}	
					DelayMs(10);
					
				while(ROTARY == 0)
				{
					if(END_ROTARY == 1 || HOOK == 0)
					break;
				}
				
				DelayMs(10);
					
				dialedNumber++ ;
			}
			
			if(HOOK == 0)
			return dialedNumber = 0; //Return Null
			
			dialedNumber-- ;
			
			if(dialedNumber == 10)
				return dialedNumber = '0';	//Return ASCII value
			
			if(dialedNumber < 10)
			{
				return (dialedNumber +48); //Return ASCII value
			}				
		}	
}
void init(void) 
{
	OSCCON = 	0b01100110;	//set frequency to 4MHz
	TRISA = 	0b00000000; //set PORTA as all output 
	TRISB = 	0b11111111; //set PORTB 
	
	//Configuare UART
	SYNC = 0;
	SPBRG = BAUD_RATE; //baud rate
	BRGH = 0;
	CREN = 1;
	
	RBPU = 0; //Enable Weak Pull Up on PORTB
	
	//Enable Interupts
	GIE = 1;
	PEIE = 1;
	RCIE = 0;
	
	ANSEL = 0x00;
	WAVE1 = 1; //Intialize wave outputs to avoid short
	WAVE2 = 0;
}

char stringCompare(const char *search, const char *find)
{
	unsigned char findSpot, searchSpot;
	unsigned char spotCharacter, searchCharacter;
	findSpot = 0;
	
	for(searchSpot = 0; ;searchSpot++)
	{
		if(find[findSpot] == '\0')
		return 1;
		
		if(search[searchSpot] == '\0')
		return 0;
		
		spotCharacter = find[findSpot];
		searchCharacter = search[searchSpot];
		
		if(spotCharacter == searchCharacter)
			findSpot++;
		else if(findSpot > 0)
			findSpot = 0;
	}

	return 0;	
}

/*A function that sends a string via the USART char by char
Input: unsigned char* word, to the char array to be sent
Output: void */
void sendString( const unsigned char *word)
{	
	while( *word)		// loop until *word == '\0' the  end of the string
		putch(*word++);	//send the first char and increment the pointer
	//putch('\n');		// finish with a newline	
}

//Blue Tooth Configuration Routine
void blueToothConfig(void)
{
	sendString("SET CONTROL CONFIG 100\n");	//Enable SCO Links
	DelayMs(500);
	sendString("SET PROFILE HFP ON\n");		//Put iWrap into HFP mode
	DelayMs(500);
	sendString("SET BT AUTH * 7589\n");		//Set Password for pairing
	DelayMs(500);
	sendString("SET BT CLASS 200408\n");	//Set Device Class (mobile handset)
	DelayMs(500);
	sendString("SET BT NAME REDPHONE\n");	//Set Device Name
	DelayMs(500);
	sendString("RESET\n");
}

/* A function which rings the phone for an incoming call and sends the "ANSWER" and "HANG UP" strings
Input: void
Output: void */
void incomingCall(void)
{
	//Does not check if caller has stopped calling :(
	int i;
	for(i =0; i <= NUMBER_RINGS &&(HOOK == 0); i++)
	{
		ring(LONG_RING);	//Ring the phone for 2 seconds
		
		if(HOOK == 0) //Check to make sure we are still on the hook
			DelayMs(4000);	//Delay for 4 Seconds
	}
	
	//DelayMs(200);
	if(HOOK == 1)
	{
		sendString("ANSWER\r\n");	
		while(HOOK == 1)
		{
			 //Wait for the user to hang up
		}
		sendString("HANGUP\r\n"); 	
		DelayMs(1000);
	}
}

//Dialing a Call: Sends a dialed number to the BT module
//Inputs: None
//Outputs: None
void call(void)
{
	char i;
	char unsigned phone_number[20];
	
	sendString("Dial yo numba\r\n");

	for(i = 0;( (i < 10) && (HOOK == 1) ) ; i++)
	{
		phone_number[i] = dialing();
	}
	sendString("ATD");
	for(i = 0; ( (i < 10) && (HOOK == 1) ); i++)
	{
		putch(phone_number[i]);
	}
	putch('\n');
	
	while(HOOK == 1); //Wait for the user to hangup
	
	sendString("HANGUP\r\n");
}	
void main(void)
{
	init_comms(); 	//Setup UART
	init(); 		//Setup IO, Interupts, and Special UART

	RINGER_HARDWARE = 0; // make sure we are in low power mode
	
	DelayMs(500);
	
//	blueToothConfig(); //Configure BT Module
	CREN = 0;
	while(1)
	{		
	//	sendString("PIC ON\r\n");			
		while(!connected)
		{
			//Clear message & setup for recieve
			for(i = 0; i<MAX_MESSAGE_SIZE; i++)
			{
				message[i] = 0;
			}
			messageIndex = 0;
			messageComplete = 0;
		//	sendString("Waiting for Phone to Connect\r\n");			
			DelayMs(500);
			CREN = 1; //Enable UART interupt for message recieve
			RCIE = 1;
			while(!messageComplete); //wait to recieve message
			RCIE = 0;
			CREN  = 0; //Process message

			if( strnicmp(message, "RING 0",6) == 0 )
			{
				connected = 1; //We are connected so change flag
				ring(SHORT_RING); //Signals User
			}
		
			//Prep ISR for next read	
			for(i = 0; i<MAX_MESSAGE_SIZE; i++)
			{
				message[i] = 0;
			}
			messageIndex = 0;
			messageComplete = 0;
		}

		//	sendString("CONNECTED TO PHONE\r\n");
			DelayMs(500);
			DelayMs(500);
			DelayMs(500);
			DelayMs(500);

			
		while(connected)
		{

			while( HOOK == 0)
			{
				for(i = 0; i<MAX_MESSAGE_SIZE; i++)
				{
					message[i] = 0;
				}
				messageIndex = 0;
				messageComplete = 0;
				
				CREN = 1;
				RCIE = 1;			
				while(!messageComplete && HOOK == 0); //check if the ISR is finished
				RCIE = 0; //Process Message
				CREN = 0;
				
				//Check if there is an incoming call
				if(strnicmp(message, "HFP 0 RING", 10) == 0 )
				{
					DelayMs(500);
					incomingCall();
				}
				//check if we disconnected aswell
				/*
				else if(strnicmp(message, "NO CARRIER 0", 12) == 0)
				{
					connected = 0;
					break; //Leave the loop so we can restablish a connection
				}
				*/

			}
			call();
		}
			
	}

}	

