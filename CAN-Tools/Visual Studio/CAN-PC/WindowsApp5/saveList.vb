Imports System.IO

Public Class saveList
    Dim pfad As String = Path.Combine(Application.StartupPath, "knownMess.xml")
    Public savetab As New DataTable ' enthält die daten welche in der Liste angezeigt werden
    Dim newrowtab As New DataTable

    'Zeile löschen
    Private Sub Button3_Click(sender As Object, e As EventArgs) Handles Button3.Click
        If SavedGrid.SelectedRows.Count > 0 Then 'löschen
            savetab.Rows(SavedGrid.SelectedRows(0).Index).Delete()
        End If
    End Sub

    'Nachricht umbenennen (Name)
    Private Sub Button2_Click(sender As Object, e As EventArgs) Handles Button2.Click
        Dim cm As CurrencyManager = CType(BindingContext(SavedGrid.DataSource), CurrencyManager)

        cm.SuspendBinding()

        If SavedGrid.SelectedRows.Count > 0 Then
            If TextBox1.Text = "" Then
                MsgBox("Name kann nicht leer sein!")
            Else
                SavedGrid.SelectedRows(0).Cells(18).Value = TextBox1.Text
            End If
        End If
        cm.ResumeBinding()
    End Sub

    'Senden jeder markierten Nachricht
    Private Sub Button1_Click(sender As Object, e As EventArgs) Handles Button1.Click
        If SavedGrid.SelectedRows.Count > 0 Then
            'senden
            Dim sp As String = " "
            Dim msgStart = "#SM "
            Dim id As String = ""
            Dim data As String = ""

            Dim reverse As Boolean = False
            If SavedGrid.SelectedRows.Count > 1 Then
                Dim probe As Integer = SavedGrid.SelectedRows(0).Index - SavedGrid.SelectedRows(1).Index
                If probe > 0 Then
                    reverse = True
                Else
                    reverse = False
                End If
            End If

            If reverse = False Then
                For y = 0 To SavedGrid.SelectedRows.Count - 1 'sende jede markierte Nachricht
                    data = ""
                    id = SavedGrid.SelectedRows(y).Cells(0).Value
                    Dim v = id.Length
                    While v < 4
                        id = "0" & id
                        v += 1
                    End While

                    Dim z As Integer = 0
                    For i = 1 To 8
                        If Not IsDBNull(SavedGrid.SelectedRows(0).Cells(i).Value) Then
                            If Not SavedGrid.SelectedRows(y).Cells(i).Value = "" Then
                                Dim byt As String = SavedGrid.SelectedRows(y).Cells(i).Value
                                If byt.Length = 1 Then
                                    byt = "0" & byt
                                End If
                                data &= sp & byt
                                z += 1
                            Else
                                Exit For
                            End If
                        End If
                    Next
                    data = " " & z & data

                    FormMain.send_msg(msgStart & id & data)
                Next
            Else
                For y = SavedGrid.SelectedRows.Count - 1 To 0 Step -1 'sende jede markierte Nachricht
                    data = ""
                    id = SavedGrid.SelectedRows(y).Cells(0).Value
                    Dim v = id.Length
                    While v < 4
                        id = "0" & id
                        v += 1
                    End While

                    Dim z As Integer = 0
                    For i = 1 To 8
                        If Not IsDBNull(SavedGrid.SelectedRows(0).Cells(i).Value) Then
                            If Not SavedGrid.SelectedRows(y).Cells(i).Value = "" Then
                                Dim byt As String = SavedGrid.SelectedRows(y).Cells(i).Value
                                If byt.Length = 1 Then
                                    byt = "0" & byt
                                End If
                                data &= sp & byt
                                z += 1
                            Else
                                Exit For
                            End If
                        End If
                    Next
                    data = " " & z & data

                    FormMain.send_msg(msgStart & id & data)
                Next
            End If
        End If

    End Sub

    'Liste speichern
    Public Sub saveList()
        savetab.WriteXml(pfad, XmlWriteMode.WriteSchema)
    End Sub

    'Liste laden
    Public Sub loadList()
        If File.Exists(pfad) Then
            savetab.ReadXml(pfad)
        End If
    End Sub

    'Form close Event
    Private Sub saveList_close(sender As Object, e As FormClosingEventArgs) Handles MyBase.Closing
        e.Cancel = True
        Me.Hide()
    End Sub

    'Form load Event
    Private Sub saveList_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        savetab.TableName = "save"
        If savetab.Columns.Count = 0 Then
            savetab.Columns.Add("CAN-ID")
            savetab.Columns.Add("Byte1")
            savetab.Columns.Add("Byte2")
            savetab.Columns.Add("Byte3")
            savetab.Columns.Add("Byte4")
            savetab.Columns.Add("Byte5")
            savetab.Columns.Add("Byte6")
            savetab.Columns.Add("Byte7")
            savetab.Columns.Add("Byte8")
            savetab.Columns.Add("Byte9")
            savetab.Columns.Add("Byte10")
            savetab.Columns.Add("Byte11")
            savetab.Columns.Add("Byte12")
            savetab.Columns.Add("Byte13")
            savetab.Columns.Add("Byte14")
            savetab.Columns.Add("Byte15")
            savetab.Columns.Add("Byte16")
            savetab.Columns.Add("Zähler")
            savetab.Columns.Add("Name")
        End If
        SavedGrid.DataSource = savetab

        SavedGrid.AutoResizeColumns(DataGridViewAutoSizeColumnMode.AllCells)
        For i = 1 To 8
            SavedGrid.Columns(i).Width = 48
        Next
        SavedGrid.Columns(18).AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill
        For i = 9 To 17
            SavedGrid.Columns(i).Visible = False
        Next
        For Each column As DataGridViewColumn In SavedGrid.Columns
            column.SortMode = DataGridViewColumnSortMode.NotSortable
        Next

        '-----------------------------------------------------------------------
        newrowtab.TableName = "new"
        If newrowtab.Columns.Count = 0 Then
            newrowtab.Columns.Add("CAN-ID")
            newrowtab.Columns.Add("Byte1")
            newrowtab.Columns.Add("Byte2")
            newrowtab.Columns.Add("Byte3")
            newrowtab.Columns.Add("Byte4")
            newrowtab.Columns.Add("Byte5")
            newrowtab.Columns.Add("Byte6")
            newrowtab.Columns.Add("Byte7")
            newrowtab.Columns.Add("Byte8")
            newrowtab.Columns.Add("Byte9")
            newrowtab.Columns.Add("Byte10")
            newrowtab.Columns.Add("Byte11")
            newrowtab.Columns.Add("Byte12")
            newrowtab.Columns.Add("Byte13")
            newrowtab.Columns.Add("Byte14")
            newrowtab.Columns.Add("Byte15")
            newrowtab.Columns.Add("Byte16")
            newrowtab.Columns.Add("Zähler")
            newrowtab.Columns.Add("Name")
        End If
        NewRowGrid.DataSource = newrowtab

        NewRowGrid.AutoResizeColumns(DataGridViewAutoSizeColumnMode.AllCells)
        For i = 1 To 8
            NewRowGrid.Columns(i).Width = 48
        Next
        NewRowGrid.Columns(18).AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill
        For i = 9 To 17
            NewRowGrid.Columns(i).Visible = False
        Next
        For Each column As DataGridViewColumn In NewRowGrid.Columns
            column.SortMode = DataGridViewColumnSortMode.NotSortable
        Next
    End Sub

    'Überträgt bei Auswahl den Namen in TextBox und in New-List
    Private Sub SavedGrid_CellContentClick(sender As Object, e As EventArgs) Handles SavedGrid.SelectionChanged, SavedGrid.CellContentClick
        If SavedGrid.SelectedRows.Count > 0 Then
            TextBox1.Text = SavedGrid.SelectedRows(0).Cells(18).Value.ToString
            Dim ascii As String = ""
            For i = 1 To 8
                If Not IsDBNull(SavedGrid.SelectedRows(0).Cells(i).Value) Then
                    If Not SavedGrid.SelectedRows(0).Cells(i).Value = "" Then
                        If SavedGrid.SelectedRows(0).Cells(i).Value = "00" Or SavedGrid.SelectedRows(0).Cells(i).Value = "0" Then
                            ascii += " "
                        Else
                            ascii += System.Convert.ToChar(System.Convert.ToUInt32(SavedGrid.SelectedRows(0).Cells(i).Value, 16))
                        End If
                    Else
                        Exit For
                    End If
                Else
                    Exit For
                End If
            Next
            TextBox2.Text = ascii
            newrowtab.Rows.Clear()
            newrowtab.ImportRow(savetab.Rows(SavedGrid.SelectedRows(0).Index))
        End If
    End Sub

    'Liste nach namen sortieren
    Private Sub Button4_Click(sender As Object, e As EventArgs) Handles Button4.Click
        savetab.DefaultView.Sort = "Name"
    End Sub

    'Liste nach IDs sortieren
    Private Sub Button5_Click(sender As Object, e As EventArgs) Handles Button5.Click
        savetab.DefaultView.Sort = "CAN-ID"
    End Sub

    'Überträgt neue Nachricht in Save-Liste
    Private Sub Button6_Click(sender As Object, e As EventArgs) Handles Button6.Click
        savetab.ImportRow(newrowtab.Rows(0))
        newrowtab.Rows.Clear()
        NewRowGrid.AllowUserToAddRows = True
    End Sub

    Private Sub NewRowGrid_CellBeginEdit(sender As Object, e As EventArgs) Handles NewRowGrid.CellBeginEdit
        NewRowGrid.AllowUserToAddRows = False
    End Sub

    'Überträgt gewählte Nachricht in OPEL Display Form
    Private Sub Button7_Click(sender As Object, e As EventArgs) Handles Button7.Click
        If SavedGrid.SelectedRows.Count > 0 Then
            displayForm.addMsg(savetab.Rows(SavedGrid.SelectedRows(0).Index))
        End If
    End Sub

End Class