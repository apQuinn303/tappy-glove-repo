/*
 * receive_test.c
 *
 *  Created on: Jan 15, 2019
 *      Author: Samuel Rainey
 */
/** example_blink_led app:
This app blinks the red LED.

For a precompiled version of this app and a tutorial on how to load this app
onto your Wixel, see the Pololu Wixel User's Guide:
http://www.pololu.com/docs/0J46
*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_com.h>
#include <radio_link.h>
#include <stdio.h>

#define TX_BUFF_LEN 100
#define RX_BUFF_LEN 4

#define DOT_TIME 1000
#define DASH_TIME 2000
#define GAP_TIME 1000
#define SPACE_TIME 1000

typedef enum { false = 0, true = 1} bool;
typedef enum MorseSymbol{INVALID = 0, DOT = 1, DASH = 2, SPACE = 3} morse_t;

morse_t txMessage[TX_BUFF_LEN] = {0};
uint8 txIndex;
bool txReadyToSend = false;

morse_t rxMessage[RX_BUFF_LEN] = {0};
uint8 rxIndex;

uint32 startTime;
uint32 symbolTime;
bool symbolInProgress = false;
bool gapInProgress = false;

int32 CODE param_blink_period_ms = 500;

uint32 lastToggle = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(0);

    if (getMs() - lastToggle >= param_blink_period_ms/2)
    {
        LED_RED(!LED_RED_STATE);
        lastToggle = getMs();
    }
}

void buzzerOn()
{
	setDigitalOutput(10,HIGH);
	//P1_0 = 1;
	//P1DIR |= 1;
	LED_YELLOW(1);
}

void buzzerOff()
{
	setDigitalOutput(10,LOW);
	//P1_0 = 0;
	//P1DIR |= 1;
	LED_YELLOW(0);
}

void initRxMessage()
{
	//This makes sure we don't receive a message of 0's.
	rxIndex = RX_BUFF_LEN;
}

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

void receiveMessage(uint8 message)
{
	uint8 i;
	rxIndex = 0;
	for(i = 0; i < RX_BUFF_LEN; i++)
	{
		rxMessage[i] = (message >> 2*i) & 0x3;
	}
}

void processSymbol(morse_t m)
{
	if(m == INVALID) return;

	startTime = getMs();
	switch(m)
	{
	case DOT:
		symbolTime = DOT_TIME;
		buzzerOn();
		break;
	case DASH:
		symbolTime = DASH_TIME;
		buzzerOn();
		break;
	case SPACE:
		symbolTime = SPACE_TIME;
	}
	symbolInProgress = true;

}



void main()
{
    systemInit();
    usbInit();

	initTxMessage();
	initRxMessage();
	receiveMessage(0x69);
	while(1)
	{
		boardService();
		//updateLeds();
		//buzzerOn();
		//delayMs(10000);
		//buzzerOff();
		if(symbolInProgress &&((getMs() - startTime) > symbolTime))
		{
			buzzerOff();
			symbolInProgress = false;
			gapInProgress = true;
			startTime = getMs();
		}

		if(gapInProgress && ((getMs() - startTime) > GAP_TIME))
		{
			gapInProgress = false;
		}

		if(rxIndex < RX_BUFF_LEN && !symbolInProgress && !gapInProgress)
		{
			processSymbol(rxMessage[rxIndex]);
			rxIndex++;
		}

		//receiveMessage(0x69);
		/*
		//if(radioComRxAvailable() && rxIndex == RX_BUFF_LEN)
		if(usbComRxAvailable() && rxIndex == RX_BUFF_LEN)
		{
			//receiveMessage(radioComRxReceiveByte());
			receiveMessage(usbComRxReceiveByte());


		}*/

		radioComTxService();
		usbComService();
	}
}

