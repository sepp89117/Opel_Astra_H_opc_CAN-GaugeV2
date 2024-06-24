Public Class filter
    Private Sub filterForm_Load(sender As Object, e As EventArgs) Handles MyBase.Load

    End Sub

    Private Function Button1_Click(sender As Object, e As EventArgs) Handles Button1.Click
        If TextBox1.Text = "" Or TextBox1.Text.Contains("0x") Then
            MsgBox("Eingabe ungültig!" & vbCrLf & "Bitte nicht im Format 0x00 eingeben")
            Me.DialogResult = DialogResult.None
        End If
    End Function
End Class