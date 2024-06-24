Imports System.IO.Ports
Imports System.Threading

Public Class FormMain
#Region "Definitionen"
    Public comportname As String = "COM4"
    Public baudrate As Integer = 115200 'COM-Port Verbindungsgeschwindigkeit
    Dim _connected As Boolean = False
    Dim IncommingData As String
    Dim dt As New DataTable
    Public mess As DataRow = dt.NewRow()
    Dim maxMonLines = 50
    Dim monitorON As Boolean = True
    Public it As DataGridViewRow
    Dim datatab As New DataTable ' enthält die daten welche in der Liste angezeigt werden
    Dim sp As String = " " 'stellt ein white space dar
    Dim ALnew As New Collection 'Puffer für Übertragung von gewählter Nachricht in Sende-Textboxen
    Dim AL As New Collection 'Kollektion der Textboxnachrichten
    Public filters As New List(Of String) 'Liste der ID-Filter
    Public MSGfilters As New List(Of DataGridViewRow)
    Public msgNew As New Collection
    Dim readThread As New Thread(AddressOf Incoming_serial)

#End Region

    'passiert bei Programmstart
    Private Sub FormMain_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        datatab.TableName = "live"

        If datatab.Columns.Count = 0 Then
            datatab.Columns.Add("CAN-ID")
            datatab.Columns.Add("Byte1")
            datatab.Columns.Add("Byte2")
            datatab.Columns.Add("Byte3")
            datatab.Columns.Add("Byte4")
            datatab.Columns.Add("Byte5")
            datatab.Columns.Add("Byte6")
            datatab.Columns.Add("Byte7")
            datatab.Columns.Add("Byte8")
            datatab.Columns.Add("Byte9")
            datatab.Columns.Add("Byte10")
            datatab.Columns.Add("Byte11")
            datatab.Columns.Add("Byte12")
            datatab.Columns.Add("Byte13")
            datatab.Columns.Add("Byte14")
            datatab.Columns.Add("Byte15")
            datatab.Columns.Add("Byte16")
            datatab.Columns.Add("Zähler")
            datatab.Columns.Add("Name")
        End If
        CanGrid.DataSource = datatab

        CanGrid.AutoResizeColumns(DataGridViewAutoSizeColumnMode.AllCells)
        For i = 1 To 17
            CanGrid.Columns(i).Width = 48
        Next
        CanGrid.Columns(18).AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill
        For i = 9 To 16
            CanGrid.Columns(i).Visible = False
        Next
        For Each column As DataGridViewColumn In CanGrid.Columns
            column.SortMode = DataGridViewColumnSortMode.NotSortable
        Next
        saveList.Show()
        saveList.Hide()
        saveList.loadList()

        AL = New Collection
        AL.Add(TextBox3)
        AL.Add(TextBox4)
        AL.Add(TextBox5)
        AL.Add(TextBox6)
        AL.Add(TextBox7)
        AL.Add(TextBox8)
        AL.Add(TextBox9)
        AL.Add(TextBox10)
        AL.Add(TextBox11)
        AL.Add(TextBox12)
        AL.Add(TextBox13)
        AL.Add(TextBox14)
        AL.Add(TextBox15)
        AL.Add(TextBox16)
        AL.Add(TextBox17)
        AL.Add(TextBox18)

    End Sub

    'passiert bei Programmende
    Private Sub FormMain_Close(sender As Object, e As EventArgs) Handles MyBase.Closing
        'Verbindung trennen
        saveList.saveList()
        ToolStripButton9.PerformClick()
    End Sub

    'Nachrichten-Liste leeren
    Private Sub ToolStripButton3_Click(sender As Object, e As EventArgs) Handles ToolStripButton3.Click, ListeLeerenToolStripMenuItem.Click
        datatab.Rows.Clear()
    End Sub

    'Serial Monitor leeren
    Private Sub ToolStripButton4_Click(sender As Object, e As EventArgs) Handles ToolStripButton4.Click, SerialMonLeerenToolStripMenuItem.Click
        MonList.Clear()
    End Sub

    'Serial connect
    Private Sub ToolStripButton8_Click(sender As Object, e As EventArgs) Handles ToolStripButton8.Click
        If FormConn.ShowDialog() = DialogResult.OK Then
            On Error GoTo errhand
            If _connected = False Then
                readThread = New Thread(AddressOf Incoming_serial)
                SerialPort.PortName = comportname
                SerialPort.BaudRate = baudrate
                AddHandler SerialPort.DataReceived, AddressOf Incoming_serial
                SerialPort.Open()
                readThread.Start()
                _connected = SerialPort.IsOpen
                Timer1.Enabled = True
            End If
            Application.DoEvents()
            System.Threading.Thread.Sleep(2500)
            If _connected = True Then
                SerialPort.Write("#SU&" & FormConn.ComboBox2.SelectedItem.ToString) 'Startet das Shield mit der gewählten BUS-Geschwindigkeit
                StatusLabel1.Text = "Verbunden"
                StatusLabel1.Image = My.Resources.if_plug_green_68983
            Else
                StatusLabel1.Text = "Nicht verbunden"
                StatusLabel1.Image = My.Resources.if_plug_red_69175
            End If
            Exit Sub
errhand:
            MsgBox(ErrorToString(), MsgBoxStyle.Critical, "Ungültiger Port")
        End If
    End Sub

    'serial trennen
    Private Sub ToolStripButton9_Click(sender As Object, e As EventArgs) Handles ToolStripButton9.Click
        While SerialPort.IsOpen
            Try
                If _connected = True Then
                    readThread.Join()
                    RemoveHandler SerialPort.DataReceived, AddressOf Incoming_serial
                    Thread.Sleep(1000)
                    SerialPort.Close()
                    MonList.Clear()
                    StatusLabel1.Text = "Nicht verbunden"
                    StatusLabel1.Image = My.Resources.if_plug_red_69175
                    _connected = False
                    Timer1.Enabled = False
                End If
            Catch
            End Try
        End While
    End Sub

    'empfangen über Port
    Private Sub Incoming_serial() ' Handles SerialPort.DataReceived
        Dim incomingData As String = Nothing

        Try
            incomingData = SerialPort.ReadLine()
            If incomingData IsNot Nothing Then
                Me.BeginInvoke(Sub() ReceivedText(incomingData))
            End If
        Catch
        End Try
    End Sub

    'zeige Nachricht auf seriellem Monitor
    Private Sub ReceivedText(ByVal [text] As String)
        IncommingData = [text]

        If monitorON = True Then
            MonList.AppendText(IncommingData)
        End If
        If IncommingData.Contains("<I") And IncommingData.Contains("/I>") And IncommingData.Contains("<Y") And IncommingData.Contains("/Y>") Then
            ReadMessages(IncommingData)
        End If
    End Sub

    'Nachrichtenverarbeitung
    Sub ReadMessages(IncommingData)
        Dim id As String
        Dim data As String()

        id = IncommingData.Remove(0, IncommingData.IndexOf("<I") + 3)
        id = id.Remove(IncommingData.IndexOf("/I>") - 4)
        id = id.Replace(" ", "")

        Dim dat As String = IncommingData.Remove(0, IncommingData.IndexOf("<Y") + 3)
        dat = dat.Remove(dat.IndexOf("/Y>"))
        data = dat.Split(New Char() {" "c})

        msgNew.Add(id)
        For Each byt In data
            msgNew.Add(byt)
        Next
        displayForm.newMsg_received(msgNew)

        Dim newRow As DataRow = datatab.NewRow()
        Dim gibts As Boolean = False
        Dim gefiltert As Boolean = False
        For Each mfilter In filters
            If id = mfilter Then
                gefiltert = True
                Exit For
            End If
        Next

        If gefiltert = False Then
            newRow(0) = id
            For i = 1 To datatab.Columns.Count() - 1
                If data.Length + 1 > i Then
                    newRow(i) = data(i - 1)
                Else
                    newRow(i) = ""
                End If
            Next
            newRow(17) = 1
            If datatab.Rows.Count <= 0 Then
                datatab.Rows.InsertAt(newRow, datatab.Rows.Count)
            Else
                For Each row In datatab.Rows
                    gibts = CompareRows(newRow, row)
                    If gibts = True Then
                        row(17) += 1 'zähler hochsetzen
                        Exit Sub
                    End If
                Next
                datatab.Rows.Add(newRow)
            End If
        End If
    End Sub

    'Vergleich von zwei Datarows
    Public Function CompareRows(ByVal newRow As DataRow, ByVal oldRow As DataRow) As Boolean
        Dim gibts As Boolean = False
        Dim array1(17)
        Dim array2(17)

        For i = 0 To newRow.ItemArray.Length - 3
            array1(i) = oldRow.ItemArray(i)
        Next
        For i = 0 To newRow.ItemArray.Length - 3
            array2(i) = newRow.ItemArray(i)
        Next

        If array1.SequenceEqual(array2) Then
            gibts = True
        Else
            gibts = False
        End If

        Return gibts
    End Function

    'Rechtsklick-Überwachung
    Private Sub Rechtsklick(ByVal sender As Object, ByVal e As MouseEventArgs) Handles CanGrid.MouseClick
        If e.Button = MouseButtons.Right Then
            Dim hti = CanGrid.HitTest(e.X, e.Y)
            CanGrid.ClearSelection()
            If hti.RowIndex > -1 Then
                CanGrid.Rows(hti.RowIndex).Selected = True
            End If
            If CanGrid.SelectedRows.Count > 0 Then
                it = CanGrid.SelectedRows(0)
                ConStrip1.Show(Me, CanGrid.PointToClient(Cursor.Position))
            End If
        End If
    End Sub

    'lädt erste markierte Nachricht in die Sende-Textboxen
    Private Sub CanGrid_SelectedIndexChanged(sender As Object, e As EventArgs) Handles CanGrid.SelectionChanged
        If CanGrid.SelectedRows.Count > 0 Then
            Try
                Dim i As Integer = CanGrid.SelectedRows.Item(0).Index

                ALnew = New Collection
                For x = 1 To 16
                    ALnew.Add(datatab.Rows(i).Item(x))
                Next
                For y = 1 To 16
                    Dim str As String = ""
                    If ALnew(y).length = 1 Then
                        str = "0" & ALnew(y)
                    Else
                        str = ALnew(y)
                    End If
                    AL(y).text = str
                Next
                Dim idtxt As String = datatab.Rows(i).Item(0)
                idtxt.Remove(0, 2)
                TextBox1.Text = idtxt
            Catch
            End Try
        End If
    End Sub

    'lädt Namen bekannter Nachrichten in die Liste der empfangenen Nachrichten
    Private Sub Timer_tick() Handles Timer1.Tick
        If datatab.Rows.Count > 0 Then
            Dim b As Boolean
            For Each row In saveList.savetab.Rows
                Dim i As Integer = 0
                For Each row2 In datatab.Rows
                    If CanGrid.Rows(i).Visible Then
                        b = CompareRows(row, row2)
                        If b Then
                            row2.Item(18) = row.item(18)
                            Exit For
                        End If
                    End If
                    i += 1
                Next
            Next
        End If
        filters_added()
    End Sub

    'öffnet bei Doppelklick die Nachricht-bearbeiten-Form
    Private Sub CanList_dc(sender As Object, e As EventArgs) Handles CanGrid.DoubleClick
        If CanGrid.SelectedRows.Count > 0 Then
            BearbeitenToolStripMenuItem1.PerformClick()
        End If
    End Sub

    'löscht Nachricht aus Empfangsliste
    Private Sub LöschenToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles LöschenToolStripMenuItem.Click
        If CanGrid.SelectedRows IsNot Nothing Then
            Dim index As Integer
            index = CanGrid.SelectedRows(0).Index
            datatab.Rows(index).Delete()
        End If
    End Sub

    'öffnet über Kontextmenü die Nachricht-bearbeiten-Form
    Private Sub BearbeitenToolStripMenuItem1_Click(sender As Object, e As EventArgs) Handles BearbeitenToolStripMenuItem1.Click
        If CanGrid.SelectedRows IsNot Nothing Then
            mess = dt.NewRow()
            mess = datatab.Rows.Item(CanGrid.SelectedRows(0).Index)
            FormEditMes.Show()
        End If
    End Sub

    'öffnet Form bekannte Nachrichten
    Private Sub ToolStripButton5_Click(sender As Object, e As EventArgs) Handles ToolStripButton5.Click, BekannteNachrichtenToolStripMenuItem.Click
        saveList.Show()
    End Sub

    'Nachricht aus Textboxen zum senden übergeben
    Private Sub Button1_Click(sender As Object, e As EventArgs) Handles Button1.Click
        Dim msgStart = "#SM "
        Dim multi As Integer
        If Not TextBox1.Text = "" & Not TextBox2.Text = "" And _connected Then
            Dim id = TextBox1.Text
            Dim v = id.Length
            While v < 4
                id = "0" & id
                v += 1
            End While
            Dim data As String = ""

            Dim z As Integer = 0
            For i = 1 To AL.Count
                If Not AL(i).text = "" Then
                    Dim byt As String = AL(i).text
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

            If Not TextBox2.Text = "" Then
                multi = TextBox2.Text
            Else
                multi = 1
            End If
            For i = 0 To multi - 1
                send_msg(msgStart & id & data) '#SM 01FF 8 30 01 32 00 34 30 36 0F
            Next
        End If
    End Sub

    'Nachricht senden
    Public Sub send_msg(ByVal msg As String)
        If _connected = True Then
            SerialPort.Write(msg) '#SM 01FF 8 30 01 32 00 34 30 36 0F
            Thread.CurrentThread.Sleep(25)
        End If

    End Sub

    'hervorheben von Nachrichten in Empfangsliste
    Private Sub HervorhebenToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles HervorhebenToolStripMenuItem.Click
        If CanGrid.SelectedRows IsNot Nothing Then
            If Not CanGrid.SelectedRows(0).Cells(0).HasStyle Then
                Dim style As New DataGridViewCellStyle
                style.Font = New Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
                style.BackColor = Color.LightPink
                CanGrid.SelectedRows(0).DefaultCellStyle = style
            Else
                CanGrid.SelectedRows(0).DefaultCellStyle = Nothing
            End If
        End If
    End Sub

    'An/Aus-Schalter für Seriellen Monitor
    Private Sub ToolStripButton6_Click(sender As Object, e As EventArgs) Handles ToolStripButton6.Click, SeriellenMonitorAnausToolStripMenuItem.Click
        If monitorON = True Then
            monitorON = False
            MonList.ForeColor = Color.Gray
        Else
            monitorON = True
            MonList.ForeColor = Color.Black
        End If
    End Sub

    'Begrenzung der Anzahl von Zeilen im Serialmonitor (val = maxMonLines)
    Private Sub MonList_TextChanged(sender As Object, e As EventArgs) Handles MonList.TextChanged
        MonList.ScrollToCaret() 'scrollt den seriellen Monitor automatisch
        If MonList.Lines.Count > maxMonLines Then
            MonList.Select(0, MonList.GetFirstCharIndexFromLine(MonList.Lines.Length - maxMonLines))
            MonList.SelectedText = ""
        End If
    End Sub

    'Programm beeanden über ToolStripMenü
    Private Sub BeendenToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles BeendenToolStripMenuItem.Click
        Me.Close()
    End Sub

    'Form mit Source für Arduino aufrufen
    Private Sub ArduinoCodeToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles ArduinoCodeToolStripMenuItem.Click
        sketchInfo.Show()
    End Sub

    'ID filter hinzufügen
    Private Sub ToolStripButton10_Click(sender As Object, e As EventArgs) Handles ToolStripButton10.Click
        If filter.ShowDialog(Me) = DialogResult.OK Then
            filters.Add(filter.TextBox1.Text)
            filters_added()
        End If
    End Sub

    'Ausführung des Filterns
    Private Sub filters_added()
        ListView1.Items.Clear()
        Dim delcount As Integer = 0
        For Each filter In filters
            For i = 0 To datatab.Rows.Count - 1 - delcount
                Try
                    If datatab.Rows(i).Item(0) = filter Then
                        datatab.Rows(i).Delete()
                        delcount += 1
                    End If
                Catch
                End Try
            Next
            ListView1.Items.Add(filter)
        Next
    End Sub

    'Gefilterte Nachrichten und IDs einblenden
    Private Sub ToolStripButton11_Click(sender As Object, e As EventArgs) Handles ToolStripButton11.Click
        Dim cm As CurrencyManager = CType(BindingContext(CanGrid.DataSource), CurrencyManager)

        cm.SuspendBinding()
        For Each row In CanGrid.Rows
            row.visible = True
        Next
        cm.ResumeBinding()

        filters.Clear() 'ID-Filter löschen
    End Sub

    'Nachrichten filtern
    Private Sub ToolStripButton12_Click(sender As Object, e As EventArgs) Handles ToolStripButton12.Click
        Dim cm As CurrencyManager = CType(BindingContext(CanGrid.DataSource), CurrencyManager)

        cm.SuspendBinding()
        For Each row In CanGrid.SelectedRows
            MSGfilters.Add(row)
            row.visible = False
        Next
        cm.ResumeBinding()
    End Sub

    'öffnet OPEL Display FOrm
    Private Sub OpelDisplayToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles OpelDisplayToolStripMenuItem.Click
        displayForm.Show()
    End Sub
End Class
