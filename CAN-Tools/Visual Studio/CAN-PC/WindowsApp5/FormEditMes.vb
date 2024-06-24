Public Class FormEditMes
    Dim dt As New DataTable

    Private Sub FormEditMes_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        dt = New DataTable
        dt.TableName = "edit"
        If dt.Columns.Count = 0 Then
            dt.Columns.Add("CAN-ID")
            dt.Columns.Add("Byte1")
            dt.Columns.Add("Byte2")
            dt.Columns.Add("Byte3")
            dt.Columns.Add("Byte4")
            dt.Columns.Add("Byte5")
            dt.Columns.Add("Byte6")
            dt.Columns.Add("Byte7")
            dt.Columns.Add("Byte8")
            dt.Columns.Add("Byte9")
            dt.Columns.Add("Byte10")
            dt.Columns.Add("Byte11")
            dt.Columns.Add("Byte12")
            dt.Columns.Add("Byte13")
            dt.Columns.Add("Byte14")
            dt.Columns.Add("Byte15")
            dt.Columns.Add("Byte16")
            dt.Columns.Add("Zähler")
            dt.Columns.Add("Name")
            'dt.PrimaryKey = New DataColumn() {dt.Columns(18)}
        End If
        Dim mess As DataRow = dt.NewRow()

        mess = FormMain.mess

        dt.ImportRow(mess)
        messGrid.DataSource = dt
        messGrid.AutoResizeColumns(DataGridViewAutoSizeColumnMode.AllCells)
        For i = 1 To 17
            messGrid.Columns(i).Width = 48
        Next
        messGrid.Columns(18).AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill
    End Sub

    Dim tosave As DataRow
    Private Sub Button2_Click(sender As Object, e As EventArgs) Handles Button2.Click
        If TextBox1.Text = "" Then
            MsgBox("Name kann nicht leer sein!")
        Else
            dt.Rows(0).Item(18) = TextBox1.Text
            tosave = dt.Rows(0) 'messGrid.Rows(0).DataBoundItem.Row 'messGrid.Rows(0).Clone
            saveList.savetab.ImportRow(tosave)
        End If
    End Sub

    Private Sub Button1_Click(sender As Object, e As EventArgs) Handles Button1.Click
        If messGrid.SelectedRows.Count > 0 Then
            'senden
            Dim sp As String = " "
            Dim msgStart = "#SM "
            Dim id As String
            Dim data As String = ""

            id = messGrid.SelectedRows(0).Cells(0).Value
            Dim v = id.Length
            While v < 4
                id = "0" & id
                v += 1
            End While

            Dim z As Integer = 0
            For i = 1 To 8
                If Not messGrid.SelectedRows(0).Cells(i).Value = "" Then
                    Dim byt As String = messGrid.SelectedRows(0).Cells(i).Value
                    If byt.Length = 1 Then
                        byt = "0" & byt
                    End If
                    data &= sp & byt
                    z += 1
                Else
                    Exit For
                End If
            Next
            data = " " & z & data

            FormMain.send_msg(msgStart & id & data) '#SM 01FF 8 30 31 32 33 34 35 36 37
            'MsgBox(msgStart & id & data)

        End If
    End Sub

    Private Sub Button3_Click(sender As Object, e As EventArgs) Handles Button3.Click
        'kopiere als dezimalzahlen zu zwischenablage, getrennt mit kommas
    End Sub

    Private Sub Button4_Click(sender As Object, e As EventArgs) Handles Button4.Click
        'kopiere als heximalzahlen zu zwischenablage, getrennt mit kommas
    End Sub
End Class