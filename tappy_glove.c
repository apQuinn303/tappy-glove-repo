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

//Comment out to run in non-USB mode.
//#define USB_MODE 1

#define TX_BUFF_LEN 100
#define RX_BUFF_LEN 4

#define DOT_TIME 1000
#define DASH_TIME 2000
#define GAP_TIME 1000
#define SPACE_TIME 1000

#define HEARTBEAT_TIME 250

uint32 lastHeartbeatTime = 0;


typedef enum { false = 0, true = 1} bool;
typedef enum MorseSymbol{INVALID = 0, DOT = 1, DASH = 2, SPACE = 3} morse_t;

morse_t txMessage[TX_BUFF_LEN] = {0};

void addToTxMessage(morse_t m);
morse_t popFromTxMessage(void);

void debugPrintByte(uint8 george);

uint8 txMessageStart = 0;
uint8 txMessageEnd = 0;


bool txReadyToSend = false;

morse_t rxMessage[RX_BUFF_LEN] = {0};
uint8 rxIndex;

uint32 startTime;
uint32 symbolTime;
bool symbolInProgress = false;
bool gapInProgress = false;


/*
 * P1_2 = DOT
 * P1_1 = DASH
 * P1_7 = SPACE
 * P1_6 = SEND
 *
 */
 
//Note: For port interrupts, MUST clear the module interrupt flag before
//the CPU interrupt flag.
ISR(P1INT, 0)
{
	if(P1IFG & 0x2) //Pin 1
	{
		P1IFG &= ~0x2; //Clear flag
		addToTxMessage(DASH);
		
	}
	
	if(P1IFG & 0x4) //Pin 2
	{
		P1IFG &= ~0x4; //Clear flag
		addToTxMessage(DOT);
	}
	
	if(P1IFG & 0x40) //Pin 6
	{
		P1IFG &= ~0x40; //Clear flag
		txReadyToSend = true;
	}
	
	if(P1IFG & 0x80) //Pin 7
	{
		P1IFG &= ~0x80; //Clear flag
		addToTxMessage(SPACE);
	}
	
	IRCON2 &= ~P1IF; //Clear CPU interrupt flag
}


uint8 nextSymbol(uint8 index)
{
	return (index + 1) % TX_BUFF_LEN;
}

void addToTxMessage(morse_t m)
{
	if(nextSymbol(txMessageEnd) != txMessageStart)
	{
		txMessage[txMessageEnd] = m;
		txMessageEnd = nextSymbol(txMessageEnd);
		
	}
}
morse_t popFromTxMessage(void)
{
	if(txMessageStart != txMessageEnd)
	{
		morse_t returnVal = txMessage[txMessageStart];
		txMessage[txMessageStart] = INVALID;
		txMessageStart = nextSymbol(txMessageStart);
		return returnVal;
	}
	else return INVALID;
}


void transmit(void)
{
	bool valid = true;
	uint8 byteToSend = 0;
	
	
	while(valid == true)
	{
		uint8 i;
		morse_t temp;
		byteToSend = 0;
		for(i = 0; i < 4; i++)
		{
			temp = popFromTxMessage();
			byteToSend |= temp << (2*i);
			
			if(temp == INVALID) valid = false;
		}
		
		if(radioComTxAvailable() && byteToSend != 0)
		{
			radioComTxSendByte(byteToSend);
			debugPrintByte(byteToSend);
		}
			
		
	}
}

void setupInterrupts(void)
{
	IRCON2 &= ~P1IF; //Clear P1 interrupt flag.
	
	//Interrupt Port 1, Pin 1, 2, 6, 7 interrupts.
	P1IEN |= 0xC6;
	
	IEN2 |= 0x10; //Enable Port 1 interrupts.
	IEN0 |= EA; //Set global interrupt enable.
}


void buzzerOn()
{
	setDigitalOutput(10,HIGH);
	LED_YELLOW(1);
}

void buzzerOff()
{
	setDigitalOutput(10,LOW);
	LED_YELLOW(0);
}

void initRxMessage()
{
	//This makes sure we don't receive a message of 0's.
	rxIndex = RX_BUFF_LEN; 
}


void debugPrintByte(uint8 george)
{
	if(usbComTxAvailable()) 
		{
			int8 j;
			usbComTxSendByte('0');
			usbComTxSendByte('b');
			
			for(j = 7; j >= 0 ; j--)
			{ 
				if((george >> j) & 0x1)
					usbComTxSendByte('1');
				else
					usbComTxSendByte('0');
			}
			
			usbComTxSendByte('\n');
		}
}

void initTxMessage()
{
	uint8 i;
	for(i = 0; i < TX_BUFF_LEN; i++)
	{
		txMessage[i] = 0;
	}
	
	txMessageStart = 0;
	txMessageEnd = 0;
	txReadyToSend = false;
}


void receiveMessage(uint8 message)
{
	uint8 i;
	rxIndex = 0;
	for(i = 0; i < RX_BUFF_LEN; i++)
	{
		uint8 george = (message >> 2*i) & 0x3;
		rxMessage[i] = george;
		debugPrintByte(george);
		
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
		break;
	}
	symbolInProgress = true;
	
}



void main()
{
    systemInit();
    usbInit();
	radioComInit();

	initTxMessage();
	initRxMessage();
	
	setupInterrupts();
	
	buzzerOff();
	
	while(1)
	{
		if(getMs() - lastHeartbeatTime > HEARTBEAT_TIME)
		{
			LED_RED_TOGGLE();
			lastHeartbeatTime = getMs();
		}
		
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

		#ifdef USB_MODE
		if(usbComRxAvailable() && rxIndex == RX_BUFF_LEN)
		{
			uint8 modeDifferentiation = usbComRxReceiveByte();
			if(modeDifferentiation == 'r')
				receiveMessage(usbComRxReceiveByte());
			else	
			{
				if(radioComTxAvailable())
				{
					//radioComTxSendByte(usbComRxReceiveByte());
				}
			}
		}
		#else
		if(radioComRxAvailable() && rxIndex == RX_BUFF_LEN 
			&& !symbolInProgress && !gapInProgress)
		{
			receiveMessage(radioComRxReceiveByte());
		}
		
			
		#endif
		
		if(txReadyToSend) 
		{
			transmit();
			txReadyToSend = false;
		}
		
		radioComTxService();
		
		//Make sure we can strap to the bootloader if necessary.
		boardService();
		
		//Run USB Communication Tasks
		usbComService();
	}
}


