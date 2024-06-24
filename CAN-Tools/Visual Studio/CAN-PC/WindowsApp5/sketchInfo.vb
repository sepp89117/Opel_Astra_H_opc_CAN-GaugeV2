Public Class sketchInfo
    Private Sub sketchInfo_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        WebBrowser1.Navigate("about:blank")

        If WebBrowser1.Document IsNot Nothing Then
            WebBrowser1.Document.Write(String.Empty)
        End If

        WebBrowser1.DocumentText = My.Resources.arducode
    End Sub
End Class