# Generates the wizard side image (a branded RackAB mockup) as a BMP.
# Replace welcome.bmp later with a real screenshot / company logo if desired.
param([string]$Out = "$PSScriptRoot\windows\welcome.bmp")

Add-Type -AssemblyName System.Drawing
$W = 328; $H = 628
$bmp = New-Object System.Drawing.Bitmap $W, $H
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.TextRenderingHint = 'ClearTypeGridFit'

# Palette (matches the plugin look)
$bg     = [System.Drawing.Color]::FromArgb(0xff,0x17,0x14,0x0f)
$panel  = [System.Drawing.Color]::FromArgb(0xff,0x21,0x1d,0x17)
$slot   = [System.Drawing.Color]::FromArgb(0xff,0x2b,0x26,0x20)
$amber  = [System.Drawing.Color]::FromArgb(0xff,0xe0,0x93,0x2e)
$green  = [System.Drawing.Color]::FromArgb(0xff,0x3f,0xbf,0x6f)
$grey   = [System.Drawing.Color]::FromArgb(0xff,0x8a,0x82,0x74)

$g.Clear($bg)

$bgBrush     = New-Object System.Drawing.SolidBrush $bg
$panelBrush  = New-Object System.Drawing.SolidBrush $panel
$slotBrush   = New-Object System.Drawing.SolidBrush $slot
$amberBrush  = New-Object System.Drawing.SolidBrush $amber
$greenBrush  = New-Object System.Drawing.SolidBrush $green
$greyBrush   = New-Object System.Drawing.SolidBrush $grey
$amberPen    = New-Object System.Drawing.Pen $amber, 2

# Header
$titleFont = New-Object System.Drawing.Font 'Segoe UI Semibold', 30, ([System.Drawing.FontStyle]::Bold)
$subFont   = New-Object System.Drawing.Font 'Segoe UI', 11
$slotFont  = New-Object System.Drawing.Font 'Consolas', 12
$g.DrawString('RackAB', $titleFont, $amberBrush, 24, 36)
$g.DrawString('ANALOG RACK  ·  A/B COMPARE', $subFont, $greyBrush, 26, 84)

# Rack slots mock
$names = @('FabFilter Pro-C 2','Waves SSL Comp','TDR Kotelnikov','Softube FET','Pro-C 2  (SOLO)','oeksound soothe2')
$y = 130
for ($i=0; $i -lt $names.Count; $i++) {
    $r = New-Object System.Drawing.Rectangle 24, $y, ($W-48), 56
    $g.FillRectangle($slotBrush, $r)
    # number
    $g.DrawString(("{0}" -f ($i+1)), $slotFont, $greyBrush, 34, ($y+18))
    # solo/active marker (green) on the SOLO one, amber bar otherwise
    if ($names[$i] -like '*SOLO*') {
        $g.FillRectangle($greenBrush, 60, ($y+8), 6, 40)
        $g.DrawString($names[$i], $slotFont, $greenBrush, 78, ($y+18))
    } else {
        $g.FillRectangle($amberBrush, 60, ($y+8), 6, 40)
        $g.DrawString($names[$i], $slotFont, (New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(0xff,0xd8,0xd0,0xc2))), 78, ($y+18))
    }
    # drag handle
    $g.DrawString([char]0x2261, $slotFont, $greyBrush, ($W-48), ($y+16))
    $y += 66
}

# Bottom accent bar
$g.FillRectangle($amberBrush, 0, ($H-6), $W, 6)

$g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Bmp)
$bmp.Dispose()
Write-Host "Wrote $Out"
