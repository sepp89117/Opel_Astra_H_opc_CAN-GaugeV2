
Private Function CAN(ByVal Unknown As SPI_CS_PIN) As MCP_CAN
End Function

'  SYSTEM: Setup routine runs on power-up or reset
Private Sub setup()
    ' Set the serial interface baud rate
    Serial.begin(38400)
    ' Initialize the CAN controller
    ' Baud rates defined in mcp_can_dfs.h
    If (CAN.begin(CAN_250KBPS) = CAN_OK) Then
        Serial.print("CAN Init OK." & vbLf & vbCrLf & vbCr)
    Else
        Serial.print("CAN Init Failed." & vbLf & vbCr)
    End If

End Sub

' end setup
' Main Loop - Arduino Entry Point
Private Sub loop()
    ' Check for a received CAN message and print it to the Serial Monitor
    SubCheckCANMessage()
    ' Check for a command from the Serial Monitor and send message as entered
    SubSerialMonitorCommand()
End Sub

' end loop
' 
' Check for CAN message and print it to the Serial Monitor
' 
Private Sub SubCheckCANMessage(ByVal Unknown As System.Void)
    ' Declarations
    Dim nMsgLen As Byte = 0
    Dim nMsgBuffer(8) As Byte

    Dim sString(4) As Char

    If (CAN.checkReceive = CAN_MSGAVAIL) Then
        ' Read the message buffer
        CAN.readMsgBuf(nMsgLen, nMsgBuffer(0))
        unsigned
        Dim nMsgID As Long = CAN.getCanId
        ' Print message ID to serial monitor
        Serial.print("Message ID : 0x")
        If (nMsgID < 16) Then
            Serial.print("0")
        End If

        Serial.print(itoa(nMsgID, sString, 16))
        Serial.print("" & vbLf & vbCr)
        ' Print data to serial monitor
        Serial.print("Data: ")
        Dim nIndex As Integer = 0
        Do While (nIndex < nMsgLen)
            Serial.print("0x")
            If (nMsgBuffer(nIndex) < 16) Then
                Serial.print("0")
            End If

            Serial.print(itoa(nMsgBuffer(nIndex), sString, 16))
            Serial.print(" ")
            nIndex = (nIndex + 1)
        Loop

        ' end for
        Serial.print(CRCR)
    End If

    ' end if
End Sub

' end subCheckCANMessage
' 
' Check for command from Serial Monitor
' 
Private Sub SubSerialMonitorCommand()
    ' Declarations
    Dim sString((MAX_CMD_LENGTH + 1)) As Char

    Dim bError As Boolean = True
    unsigned
    Dim nMsgID As Long = 65535
    Dim nMsgLen As Byte = 0
    Dim nMsgBuffer(8) As Byte

    ' Check for command from Serial Monitor
    Dim nLen As Integer = nFctReadSerialMonitorString(sString)
    If (nLen > 0) Then
        ' A string was received from serial monitor
        If (strncmp(sString, "#SM ", 4) = 0) Then
            ' The first 4 characters are acceptable
            ' We need at least 10 characters to read the ID and data number
            If (strlen(sString) >= 10) Then
                ' Determine message ID and number of data bytes
                nMsgID = lFctCStringLong(sString(4), 4)
                nMsgLen = CType(nFctCStringInt(sString(9), 1), Byte)
                If ((nMsgLen >= 0) _
                            AndAlso (nMsgLen <= 8)) Then
                    ' Check if there are enough data entries
                    Dim nStrLen As Integer = (10 _
                                + (nMsgLen * 3))
                    ' Expected msg length
                    If (strlen(sString) >= nStrLen) Then
                        Dim nPointer As Integer
                        Dim nIndex As Integer = 0
                        Do While (nIndex < nMsgLen)
                            nPointer = (nIndex * 3)
                            ' Blank character plus two numbers
                            nMsgBuffer(nIndex) = CType(nFctCStringInt(sString((nPointer + 11)), 2), Byte)
                            nIndex = (nIndex + 1)
                        Loop

                        ' end for
                        ' Reset the error flag
                        bError = False
                        ' Everything okay; send the message
                        CAN.sendMsgBuf(nMsgID, CAN_STDID, nMsgLen, nMsgBuffer)
                        ' Repeat the entry on the serial monitor
                        Serial.print(sString)
                        Serial.print(CRCR)
                    End If

                    ' end if
                End If

                ' end if
            End If

            ' end if
        End If

        ' end if
        ' Check for entry error
        If (bError = True) Then
            Serial.print(" ? ? ? : ")
            Serial.print(sString)
            Serial.print(CR)
        End If

    End If

    ' end if
End Sub

' end SubSerialMonitorCommand
' 
' Read message from Serial Monitor
' 
' Returns string length
'
Private Function nFctReadSerialMonitorString(ByVal sString As Char) As Byte
    ' Declarations
    Dim nCount As Byte
    nCount = 0
    If (Serial.available > 0) Then
        Serial.setTimeout(100)
        nCount = Serial.readBytes(sString, MAX_CMD_LENGTH)
    End If

    ' end if
    ' Terminate the string
    sString(nCount) = 0
    Return nCount
End Function

' end nFctReadSerialMonitorString
' 
' Convert string into int
' 
' Note: nLen MUST be between 1 and 4
'
' Returns integer value (-1 indicates an error in the string)
'
Private Function nFctCStringInt(ByVal sString As Char, ByVal nLen As Integer) As Integer
    ' Declarations
    Dim nNum As Integer
    Dim nRetCode As Integer = 0
    ' Check the string length
    If (strlen(sString) < nLen) Then
        nRetCode = -1
    Else
        ' String length okay; convert number
        Dim nShift As Integer = 0
        Dim nIndex As Integer = (nLen - 1)
        Do While (nIndex >= 0)
            If ((sString(nIndex) >= Microsoft.VisualBasic.ChrW(48)) _
                        AndAlso (sString(nIndex) <= Microsoft.VisualBasic.ChrW(57))) Then
                nNum = int((sString(nIndex) - Microsoft.VisualBasic.ChrW(48)))
            ElseIf ((sString(nIndex) >= Microsoft.VisualBasic.ChrW(65)) _
                        AndAlso (sString(nIndex) <= Microsoft.VisualBasic.ChrW(70))) Then
                nNum = (int((sString(nIndex) - Microsoft.VisualBasic.ChrW(65))) + 10)
            Else
                GoTo nFctCStringInt_Ret
            End If

            nNum = (nNum _
                        + ((nShift + 1) _
                        * 4))
            nRetCode = (nRetCode + nNum)
            nIndex = (nIndex - 1)
        Loop

        ' end for
    End If

    ' end else
    ' Return the result
nFctCStringInt_Ret:
    Return nRetCode
End Function

Private Function lFctCStringLong(ByVal sString As Char, ByVal nLen As Integer) As Long
    ' Declarations
    unsigned
    Dim nNum As Long
    unsigned
    Dim nRetCode As Long = 0
    ' Check the string length
    If (strlen(sString) < nLen) Then
        nRetCode = -1
    Else
        ' String length okay; convert number
        unsigned
        Dim nShift As Long = 0
        Dim nIndex As Integer = (nLen - 1)
        Do While (nIndex >= 0)
            If ((sString(nIndex) >= Microsoft.VisualBasic.ChrW(48)) _
                        AndAlso (sString(nIndex) <= Microsoft.VisualBasic.ChrW(57))) Then
                nNum = int((sString(nIndex) - Microsoft.VisualBasic.ChrW(48)))
            ElseIf ((sString(nIndex) >= Microsoft.VisualBasic.ChrW(65)) _
                        AndAlso (sString(nIndex) <= Microsoft.VisualBasic.ChrW(70))) Then
                nNum = (int((sString(nIndex) - Microsoft.VisualBasic.ChrW(65))) + 10)
            Else
                GoTo lFctCStringLong_Ret
            End If

            nNum = (nNum _
                        + ((nShift + 1) _
                        * 4))
            nRetCode = (nRetCode + nNum)
            nIndex = (nIndex - 1)
        Loop

        ' end for
    End If

    ' end else
    ' Return the result
lFctCStringLong_Ret:
    Return nRetCode
End Function