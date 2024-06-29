/*
 * Dieser Code ist zur Verwendung mit einem MCP2551 CAN Modul vorgesehen, welches über Serial am Arduino angebudnen wird in Verbindung mit der Software "CAN-PC"
 * CAN Bord: https://docs.longan-labs.cc/1030001/
 * Library: https://github.com/Longan-Labs/Serial_CAN_Arduino
*/

#include <Serial_CAN_Module.h>
#include <SoftwareSerial.h>
#include <stdlib.h>
#include "Arduino.h"

#define MAX_CMD_LENGTH 60
#define CAN_MSG_LEN 8
#define CAN_TX 2				 // tx of serial can module connect to D2
#define CAN_RX 3				 // rx of serial can module connect to D3
#define CAN_MODUL_COM_BAUD 9600	 // Die Baudrate für die Kommunikation zwischen CAN-Modul und Arduino
#define SOFTWARE_COM_BAUD 115200 // Die Baudrate für die Kommunikation zwischen Arduino und der Software

bool conn = false;
Serial_CAN can;

void setup()
{
	// Die Serial-Schnittstelle ist für die Kommunikation mit der Software
	Serial.begin(SOFTWARE_COM_BAUD);

	// can ist die Instanz für die Kommunikation mit dem CAN-Modul
	can.begin(CAN_TX, CAN_RX, CAN_MODUL_COM_BAUD);
}

void loop()
{
	// Check for a received CAN message and print it to the Serial Monitor
	if (conn == true)
		SubCheckCANMessage();

	// Check for a command from the Serial Monitor and send message as entered
	SubSerialMonitorCommand();
}

// ––––––––––––––––––––––––
// Check for CAN message and print it to the Serial Monitor
// ––––––––––––––––––––––––
void SubCheckCANMessage()
{
	unsigned long canId = 0;
	unsigned char buf[CAN_MSG_LEN];

	if (can.recv(&canId, buf)) // check if data coming
	{
		Serial.print("<I ");
		Serial.print(canId, HEX);
		Serial.print(" /I>");

		Serial.print("<Y ");
		for (int i = 0; i < CAN_MSG_LEN; i++) // print the data
		{
			Serial.print(buf[i], HEX);
			Serial.print(" ");
		}
		Serial.print("/Y> \r\n");
	}
}

// ––––––––––––––––––––––––
// Check for command from Serial Monitor
// ––––––––––––––––––––––––
void SubSerialMonitorCommand()
{
	// Declarations
	char sString[MAX_CMD_LENGTH + 1];
	bool bError = true;
	unsigned long nMsgID = 0xFFFF;
	byte nMsgLen = 0;
	byte nMsgBuffer[8];
	// Check for command from Serial Monitor
	int nLen = nFctReadSerialMonitorString(sString);
	if (nLen > 0)
	{
		// A string was received from serial monitor
		if (strncmp(sString, "#SM ", 4) == 0)
		{
			// The first 4 characters are acceptable
			// We need at least 10 characters to read the ID and data number
			if (strlen(sString) >= 10)
			{
				// Determine message ID and number of data bytes
				nMsgID = lFctCStringLong(&sString[4], 4);
				nMsgLen = (byte)nFctCStringInt(&sString[9], 1);
				if (nMsgLen >= 0 && nMsgLen <= 8)
				{
					// Check if there are enough data entries
					int nStrLen = 10 + nMsgLen * 3; // Expected msg length
					if (strlen(sString) >= nStrLen) // Larger length is acceptable
					{
						int nPointer;
						for (int nIndex = 0; nIndex < nMsgLen; nIndex++)
						{
							nPointer = nIndex * 3; // Blank character plus two numbers
							nMsgBuffer[nIndex] = (byte)nFctCStringInt(&sString[nPointer + 11], 2);
						} // end for
						  // Reset the error flag
						bError = false;

						// Everything okay; send the message
						can.send(nMsgID, 0, 0, nMsgLen, nMsgBuffer);

						// Repeat the entry on the serial monitor
						Serial.print(sString);
						Serial.print("\r\n");
					} // end if
				} // end if
			} // end if
		}
		else if (strncmp(sString, "#SU&", 4) == 0)
		{
			String c(sString);
			String str = c.substring(4);

			// Initialize the CAN controller
			if (str == "CAN_5KBPS")
			{
				setCanRate(CAN_RATE_5);
			}
			else if (str == "CAN_10KBPS")
			{
				setCanRate(CAN_RATE_10);
			}
			else if (str == "CAN_20KBPS")
			{
				setCanRate(CAN_RATE_20);
			}
			else if (str == "CAN_25KBPS")
			{
				setCanRate(CAN_RATE_25);
			}
			else if (str == "CAN_31K25BPS")
			{
				setCanRate(CAN_RATE_31_2);
			}
			else if (str == "CAN_33KBPS")
			{
				setCanRate(CAN_RATE_33);
			}
			else if (str == "CAN_40KBPS")
			{
				setCanRate(CAN_RATE_40);
			}
			else if (str == "CAN_50KBPS")
			{
				setCanRate(CAN_RATE_50);
			}
			else if (str == "CAN_80KBPS")
			{
				setCanRate(CAN_RATE_80);
			}
			else if (str == "CAN_83K3BPS")
			{
				setCanRate(CAN_RATE_83_3);
			}
			else if (str == "CAN_95KBPS")
			{
				setCanRate(CAN_RATE_95);
			}
			else if (str == "CAN_100KBPS")
			{
				setCanRate(CAN_RATE_100);
			}
			else if (str == "CAN_125KBPS")
			{
				setCanRate(CAN_RATE_125);
			}
			else if (str == "CAN_200KBPS")
			{
				setCanRate(CAN_RATE_200);
			}
			else if (str == "CAN_250KBPS")
			{
				setCanRate(CAN_RATE_250);
			}
			else if (str == "CAN_500KBPS")
			{
				setCanRate(CAN_RATE_500);
			}
			else if (str == "CAN_666KBPS")
			{
				setCanRate(CAN_RATE_666);
			}
			else if (str == "CAN_1000KBPS")
			{
				setCanRate(CAN_RATE_1000);
			}
		}
	}
}

void setCanRate(unsigned char rate)
{
	while (!can.canRate(rate)) // init can bus : baudrate = 500k
	{
		Serial.println("CAN BUS Shield init fail!");
		Serial.println("Init CAN BUS Shield again...");
		delay(100);
	}

	Serial.println("CAN BUS Shield init ok!");
	conn = true;
}

// ––––––––––––––––––––––––
// Read message from Serial Monitor
// ––––––––––––––––––––––––
// Returns string length
//
byte nFctReadSerialMonitorString(char *sString)
{
	// Declarations
	byte nCount;
	nCount = 0;
	if (Serial.available() > 0)
	{
		Serial.setTimeout(100);
		nCount = Serial.readBytes(sString, MAX_CMD_LENGTH);
	} // end if
	  // Terminate the string
	sString[nCount] = 0;
	return nCount;
} // end nFctReadSerialMonitorString

// ––––––––––––––––––––––––
// Convert string into int
// ––––––––––––––––––––––––
// Note: nLen MUST be between 1 and 4
//
// Returns integer value (-1 indicates an error in the string)
//
int nFctCStringInt(char *sString, int nLen)
{
	// Declarations
	int nNum;
	int nRetCode = 0;
	// Check the string length
	if (strlen(sString) < nLen)
		nRetCode = -1;
	else
	{
		// String length okay; convert number
		int nShift = 0;
		for (int nIndex = nLen - 1; nIndex >= 0; nIndex--)
		{
			if (sString[nIndex] >= '0' && sString[nIndex] <= '9')
				nNum = int(sString[nIndex] - '0');
			else if (sString[nIndex] >= 'A' && sString[nIndex] <= 'F')
				nNum = int(sString[nIndex] - 'A') + 10;
			else
				goto nFctCStringInt_Ret;
			nNum = nNum << (nShift++ * 4);
			nRetCode = nRetCode + nNum;
		} // end for
	} // end else
	  // Return the result
nFctCStringInt_Ret:
	return nRetCode;
} // end nFctCStringInt

// ––––––––––––––––––––––––
// Convert string into unsigned long
// ––––––––––––––––––––––––
// Note: nLen MUST be between 1 and 4
//
// Returns integer value (-1 indicates an error in the string)
//
unsigned long lFctCStringLong(char *sString, int nLen)
{
	// Declarations
	unsigned long nNum;
	unsigned long nRetCode = 0;
	// Check the string length
	if (strlen(sString) < nLen)
		nRetCode = -1;
	else
	{
		// String length okay; convert number
		unsigned long nShift = 0;
		for (int nIndex = nLen - 1; nIndex >= 0; nIndex--)
		{
			if (sString[nIndex] >= '0' && sString[nIndex] <= '9')
				nNum = int(sString[nIndex] - '0');
			else if (sString[nIndex] >= 'A' && sString[nIndex] <= 'F')
				nNum = int(sString[nIndex] - 'A') + 10;
			else
				goto lFctCStringLong_Ret;
			nNum = nNum << (nShift++ * 4);
			nRetCode = nRetCode + nNum;
		} // end for
	} // end else
	  // Return the result
lFctCStringLong_Ret:
	return nRetCode;
} // end lFctCStringLong