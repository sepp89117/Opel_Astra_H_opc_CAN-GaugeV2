Public Class FormConn

    Private Sub FormConn_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        ' Show all available COM ports.
        Dim PortList As New List(Of String)

        If ComboBox1.Items.Count <= 0 Then
            For Each sp As String In My.Computer.Ports.SerialPortNames
                PortList.Add(sp)
                ComboBox1.Items.Add(sp)
            Next
        Else
            ComboBox1.Items.Clear()
            For Each sp As String In My.Computer.Ports.SerialPortNames
                PortList.Add(sp)
                ComboBox1.Items.Add(sp)
            Next
        End If

        ComboBox1.SelectedIndex = ComboBox1.Items.Count - 1

        ComboBox2.SelectedIndex = 15

        ComboBox3.SelectedIndex = 10
    End Sub

    Private Sub Form_Close() Handles Me.Closing
        FormMain.comportname = ComboBox1.SelectedItem.ToString
        FormMain.portLabel.Text = ComboBox1.SelectedItem.ToString
        FormMain.baudrate = ComboBox3.SelectedItem.ToString
        FormMain.ToolStripStatusLabel2.Text = ComboBox2.Text.Remove(0, 4).ToLower
    End Sub
End Class