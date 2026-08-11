# Generate radio button indicator icons (32x32) for QSS
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot "..\src\gui\resources\icons"
$outDir = (Resolve-Path $outDir).Path

function New-RadioIcon([string]$path, [bool]$checkedState) {
    $bmp = New-Object System.Drawing.Bitmap(32, 32)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    # Outer circle: white fill + border
    $borderColor = if ($checkedState) { [System.Drawing.Color]::FromArgb(30, 136, 229) } else { [System.Drawing.Color]::FromArgb(144, 164, 174) }
    $pen = New-Object System.Drawing.Pen($borderColor, 3)
    $white = [System.Drawing.Brushes]::White
    $g.FillEllipse($white, 2, 2, 28, 28)
    $g.DrawEllipse($pen, 2, 2, 28, 28)

    if ($checkedState) {
        # Center blue dot
        $blue = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(30, 136, 229))
        $g.FillEllipse($blue, 10, 10, 12, 12)
        $blue.Dispose()
    }

    $pen.Dispose()
    $g.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

New-RadioIcon (Join-Path $outDir "radio_off.png") $false
New-RadioIcon (Join-Path $outDir "radio_on.png") $true

Write-Host "Radio icons generated in $outDir"
