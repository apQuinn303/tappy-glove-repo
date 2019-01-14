/** example_blink_led app:
This app blinks the red LED.

For a precompiled version of this app and a tutorial on how to load this app
onto your Wixel, see the Pololu Wixel User's Guide:
http://www.pololu.com/docs/0J46
*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

#define TX_BUFF_LEN 100

typedef enum { false = 0, true = 1} bool;
typedef enum MorseSymbol{INVALID = 0, DOT = 1, DASH = 2, SPACE = 3} morse_t;

morse_t txMessage[TX_BUFF_LEN] = {0};
uint8 txIndex;
bool txReadyToSend = false;

void initTxMessage()
{
	uint8 i;
	for(i = 0; i < TX_BUFF_LEN; i++)
	{
		txMessage[i] = 0;
	}
	txIndex = 0;
	txReadyToSend = false;
}

void newTxSymbol(morse_t m)
{
	txMessage[txIndex] = m;
	txIndex++;
}

void main()
{
    systemInit();
    usbInit();

	initTxMessage();
	
}
