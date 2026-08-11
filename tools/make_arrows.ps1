# Generates spin/combo arrow icons used by QSS subcontrols:
#   spin_up.png / spin_down.png (16x16) for QSpinBox/QTimeEdit up/down buttons
#   combo_down.png (20x20) for QComboBox drop-down button
Add-Type -AssemblyName System.Drawing

$icons = Join-Path $PSScriptRoot '..\src\gui\resources\icons'
$brush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 84, 110, 122)) # #546E7A

function Save-Triangle([string]$path, [int]$size, [System.Drawing.Point[]]$pts) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.FillPolygon($brush, $pts)
    $g.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "written: $path"
}

Save-Triangle (Join-Path $icons 'spin_up.png') 16 ([System.Drawing.Point[]]@(
    [System.Drawing.Point]::new(8, 4),
    [System.Drawing.Point]::new(3, 11),
    [System.Drawing.Point]::new(13, 11)))

Save-Triangle (Join-Path $icons 'spin_down.png') 16 ([System.Drawing.Point[]]@(
    [System.Drawing.Point]::new(8, 12),
    [System.Drawing.Point]::new(3, 5),
    [System.Drawing.Point]::new(13, 5)))

Save-Triangle (Join-Path $icons 'combo_down.png') 20 ([System.Drawing.Point[]]@(
    [System.Drawing.Point]::new(10, 13),
    [System.Drawing.Point]::new(4, 6),
    [System.Drawing.Point]::new(16, 6)))

$brush.Dispose()
