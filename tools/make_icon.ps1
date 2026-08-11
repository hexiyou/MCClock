# Generates the MCClock flat-style application icon (PNG + ICO)
# Output: resources/icons/app.png and resources/icons/app.ico
param(
    [string]$OutDir = (Join-Path $PSScriptRoot "..\src\gui\resources\icons")
)

Add-Type -AssemblyName System.Drawing

$size = 256
$bmp = New-Object System.Drawing.Bitmap($size, $size)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::Transparent)

# Rounded blue background (#1E88E5)
$bg = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 30, 136, 229))
$radius = 48
$path = New-Object System.Drawing.Drawing2D.GraphicsPath
$path.AddArc(0, 0, $radius, $radius, 180, 90)
$path.AddArc($size - $radius, 0, $radius, $radius, 270, 90)
$path.AddArc($size - $radius, $size - $radius, $radius, $radius, 0, 90)
$path.AddArc(0, $size - $radius, $radius, $radius, 90, 90)
$path.CloseFigure()
$g.FillPath($bg, $path)

$white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$whitePen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 12)
$whitePen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$whitePen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round

# Bells (two arcs above the clock face)
$bellPen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 14)
$bellPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$bellPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$g.DrawLine($bellPen, 70, 62, 96, 44)
$g.DrawLine($bellPen, 186, 62, 160, 44)

# Clock face (white ring)
$faceRect = New-Object System.Drawing.Rectangle(52, 64, 152, 152)
$g.DrawEllipse($whitePen, $faceRect)

# Hour marks
$markPen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 8)
$cx = 128; $cy = 140; $rOuter = 64; $rInner = 52
foreach ($i in 0..11) {
    $angle = $i * 30.0 * [Math]::PI / 180.0
    $x1 = $cx + $rInner * [Math]::Sin($angle)
    $y1 = $cy - $rInner * [Math]::Cos($angle)
    $x2 = $cx + $rOuter * [Math]::Sin($angle)
    $y2 = $cy - $rOuter * [Math]::Cos($angle)
    $g.DrawLine($markPen, [single]$x1, [single]$y1, [single]$x2, [single]$y2)
}

# Hands (10:10 style)
$handPen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 12)
$handPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$handPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$g.DrawLine($handPen, $cx, $cy, [single]($cx - 26), [single]($cy - 26))   # hour hand
$g.DrawLine($handPen, $cx, $cy, [single]($cx + 22), [single]($cy - 38))   # minute hand
$g.FillEllipse($white, ($cx - 7), ($cy - 7), 14, 14)

# Feet
$g.DrawLine($bellPen, 82, 214, 66, 234)
$g.DrawLine($bellPen, 174, 214, 190, 234)

$g.Dispose()

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$pngPath = Join-Path $OutDir "app.png"
$bmp.Save($pngPath, [System.Drawing.Imaging.ImageFormat]::Png)

# Build ICO with a single 256x256 PNG entry (supported since Windows Vista)
$pngBytes = [System.IO.File]::ReadAllBytes($pngPath)
$icoPath = Join-Path $OutDir "app.ico"
$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)
$bw.Write([uint16]0)              # reserved
$bw.Write([uint16]1)              # type: icon
$bw.Write([uint16]1)              # image count
$bw.Write([byte]0)                # width (0 = 256)
$bw.Write([byte]0)                # height (0 = 256)
$bw.Write([byte]0)                # palette
$bw.Write([byte]0)                # reserved
$bw.Write([uint16]1)              # planes
$bw.Write([uint16]32)             # bpp
$bw.Write([uint32]$pngBytes.Length)
$bw.Write([uint32]22)             # data offset (6 + 16)
$bw.Write($pngBytes)
[System.IO.File]::WriteAllBytes($icoPath, $ms.ToArray())
$bw.Dispose()
$ms.Dispose()
$bmp.Dispose()

Write-Host "Generated: $pngPath"
Write-Host "Generated: $icoPath"
