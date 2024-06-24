Public Class displayForm
    Dim msg As New DataTable
    Dim row1 As String
    Dim row2 As String
    Dim row3 As String
    Dim row4 As String
    Dim mydata As New Collection
    Dim datalength As Integer
    Dim isStarted As Boolean = False
    Dim compArray As New Collection 'teil der nachricht, welcher zur identifizierung genutzt werden soll
    Dim dataArray As New Collection 'teil der Nachricht, welcher die Daten enthält
    Dim sendMsg(10) As String

    Private Sub displayForm_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        'max char count = 10
        msg.TableName = "save"
        If msg.Columns.Count = 0 Then
            msg.Columns.Add("CAN-ID")
            msg.Columns.Add("Byte1")
            msg.Columns.Add("Byte2")
            msg.Columns.Add("Byte3")
            msg.Columns.Add("Byte4")
            msg.Columns.Add("Byte5")
            msg.Columns.Add("Byte6")
            msg.Columns.Add("Byte7")
            msg.Columns.Add("Byte8")
            msg.Columns.Add("Byte9")
            msg.Columns.Add("Byte10")
            msg.Columns.Add("Byte11")
            msg.Columns.Add("Byte12")
            msg.Columns.Add("Byte13")
            msg.Columns.Add("Byte14")
            msg.Columns.Add("Byte15")
            msg.Columns.Add("Byte16")
            msg.Columns.Add("Zähler")
            msg.Columns.Add("Name")
        End If
        DataRowGrid.DataSource = msg
    End Sub

    Public Sub addMsg(ByVal row As DataRow)
        msg.Rows.Clear()
        msg.ImportRow(row)
    End Sub

    Public Sub newMsg_received(ByVal msg As Collection)
        If isStarted = True Then

            dataArray.Add(msg(0)) 'Id

            If CheckBox1.CheckState = True Then
                dataArray.Add(msg(1))
            End If

            If CheckBox2.CheckState = True Then
                dataArray.Add(msg(2))
            End If

            If CheckBox3.CheckState = True Then
                dataArray.Add(msg(3))
            End If

            If CheckBox4.CheckState = True Then
                dataArray.Add(msg(4))
            End If

            If CheckBox5.CheckState = True Then
                dataArray.Add(msg(5))
            End If

            If CheckBox6.CheckState = True Then
                dataArray.Add(msg(6))
            End If

            If CheckBox7.CheckState = True Then
                dataArray.Add(msg(7))
            End If

            If CheckBox8.CheckState = True Then
                dataArray.Add(msg(8))
            End If
            'ab hier haben wir den zur Identifizierung benötigten Teil der eingegangenen Nachricht im dataArray
            '----------------------------------------------------

            'Wenn Abfrage gestartet und die eingegangene Nachricht als die gesuchte identifiziert ist, dann werden die relevanten Daten herausgezogen.
            If isStarted Then
                If dataArray.Equals(compArray) Then

                Else
                    dataArray.Clear()
                End If
            End If



                If CheckBox9.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(1).Value)
            End If

            If CheckBox10.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(2).Value)
            End If

            If CheckBox11.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(3).Value)
            End If

            If CheckBox12.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(4).Value)
            End If

            If CheckBox13.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(5).Value)
            End If

            If CheckBox14.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(6).Value)
            End If

            If CheckBox15.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(7).Value)
            End If

            If CheckBox16.CheckState = True Then
                mydata.Add(DataRowGrid.Rows(0).Cells(8).Value)
            End If

        End If
    End Sub

    Private Sub Button1_Click(sender As Object, e As EventArgs) Handles Button1.Click
        'Start
        isStarted = True


        compArray.Add(DataRowGrid.Rows(0).Cells(0).Value)       'Id

        If CheckBox1.CheckState = True Then
            compArray.Add(DataRowGrid.Rows(0).Cells(1).Value)
        End If

        If CheckBox2.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(2).Value)
        End If

        If CheckBox3.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(3).Value)
        End If

        If CheckBox4.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(4).Value)
        End If

        If CheckBox5.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(5).Value)
        End If

        If CheckBox6.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(6).Value)
        End If

        If CheckBox7.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(7).Value)
        End If

        If CheckBox8.CheckState = True Then
            mydata.Add(DataRowGrid.Rows(0).Cells(8).Value)
        End If

        datalength = compArray.Count
        'ab hier haben wir die daten als Identifizierung im Array

        row1 = "#SM 06C1 8 23 00 32 00 34 00 36 00"
    End Sub

    Private Sub Button2_Click(sender As Object, e As EventArgs) Handles Button2.Click
        isStarted = False
    End Sub
End Class