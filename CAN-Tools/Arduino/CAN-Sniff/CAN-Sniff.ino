#include <mcp_can.h>
#include <mcp_can_dfs.h>

#include <stdlib.h>
#include "Arduino.h"
#include <SPI.h>

#define MAX_CMD_LENGTH 60
bool conn = false;
MCP_CAN CAN(10);

void setup()
{
	// Set the serial interface baud rate
	Serial.begin(115200);
}

void loop()
{
	// Check for a received CAN message and print it to the Serial Monitor
	if (conn == true)
	{
		SubCheckCANMessage();
	}

	// Check for a command from the Serial Monitor and send message as entered
	SubSerialMonitorCommand();
}

// ––––––––––––––––––––––––
// Check for CAN message and print it to the Serial Monitor
// ––––––––––––––––––––––––
void SubCheckCANMessage()
{
	unsigned char len = 0;
	unsigned char buf[8];

	if (CAN_MSGAVAIL == CAN.checkReceive()) // check if data coming
	{
		CAN.readMsgBuf(&len, buf); // read data,  len: data length, buf: data buf

		unsigned int canId = CAN.getCanId();

		Serial.print("<I ");
		Serial.print(canId, HEX);
		Serial.print(" /I>");

		Serial.print("<Y ");
		for (int i = 0; i < len; i++) // print the data
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
							nMsgBuffer[nIndex] =
								(byte)nFctCStringInt(&sString[nPointer + 11], 2);
						} // end for
						  // Reset the error flag
						bError = false;
						// Everything okay; send the message
						CAN.sendMsgBuf(nMsgID, CAN_STDID, nMsgLen, nMsgBuffer);
						// Repeat the entry on the serial monitor
						Serial.print(sString);
						Serial.print("\r\n");
					} // end if
				} // end if
			} // end if
		}
		else if (strncmp(sString, "#SU&", 4) == 0)
		{
			bError = false;

			String c(sString);
			String str = c.substring(4);

			// Initialize the CAN controller
			if (str == "CAN_5KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_5KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_10KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_10KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_20KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_20KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_25KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_25KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_31K25BPS")
			{
				while (CAN_OK != CAN.begin(CAN_31K25BPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_33KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_33KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_40KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_40KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_50KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_50KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_80KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_80KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_83K3BPS")
			{
				while (CAN_OK != CAN.begin(CAN_83K3BPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_95KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_95KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_100KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_100KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_125KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_125KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_200KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_200KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_250KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_250KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_500KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_666KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_666KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}
			if (str == "CAN_1000KBPS")
			{
				while (CAN_OK != CAN.begin(CAN_1000KBPS)) // init can bus : baudrate = 500k
				{
					Serial.println("CAN BUS Shield init fail");
					Serial.println(" Init CAN BUS Shield again");
					delay(100);
				}
				Serial.println("CAN BUS Shield init ok!");
			}

		} // end if

		// Check for entry error
		if (bError == true)
		{
			Serial.print(" ? ? ? : ");
			Serial.print(sString);
			Serial.print("\r\n");
		}
		else
		{
			conn = true;
		}
	} // end if
} // end SubSerialMonitorCommand

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