/**
*@file usart.c
*@brief USART source file 
*@author Dirk Dubois
*@version 4.0
*@date November 22, 2011
*/

/*Includes*/
#include <htc.h>
#include <stdio.h>
#include "usart.h"

void putch(unsigned char byte) 
{
	/* output one byte */
	while(!TRMT)	/* set when register is empty */
		continue;
	TXREG = byte;

}

unsigned char getch() {
	/* retrieve one byte */
	while(!RCIF)	/* set when register is not empty */
		continue;
	return RCREG;	
}

unsigned char getche(void)
{
	unsigned char c;
	putch(c = getch());
	return c;
}

