# Generates icons/check.png: a white tick mark on transparent background,
# used by QSS QCheckBox::indicator:checked (drawn over the blue fill).
Add-Type -AssemblyName System.Drawing

$out = Join-Path $PSScriptRoot '..\src\gui\resources\icons\check.png'

$bmp = New-Object System.Drawing.Bitmap 32, 32
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::Transparent)

$pen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 5)
$pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round

$pts = [System.Drawing.Point[]]@(
    [System.Drawing.Point]::new(7, 17),
    [System.Drawing.Point]::new(13, 23),
    [System.Drawing.Point]::new(25, 9)
)
$g.DrawLines($pen, $pts)

$pen.Dispose()
$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host "check.png written: $out"
