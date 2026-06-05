Add-Type -AssemblyName System.Drawing

$w = 96
$h = 128
$fmt = [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
$bmp = New-Object System.Drawing.Bitmap -ArgumentList $w, $h, $fmt
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::Transparent)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

function Col($hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
function RectPx($x, $y, $ww, $hh, $col) {
    $b = New-Object System.Drawing.SolidBrush (Col $col)
    $g.FillRectangle($b, $x, $y, $ww, $hh)
    $b.Dispose()
}
function LinePx($x1, $y1, $x2, $y2, $col, $sz = 1) {
    $p = New-Object System.Drawing.Pen -ArgumentList (Col $col), $sz
    $g.DrawLine($p, $x1, $y1, $x2, $y2)
    $p.Dispose()
}

# Tail and long blue-white hair masses.
RectPx 64 85 18 12 '#ffffff'; RectPx 66 95 18 8 '#eaf6ff'; RectPx 78 92 7 7 '#ffffff'
RectPx 61 92 6 6 '#102044'; RectPx 63 96 4 4 '#87c9ff'; RectPx 81 97 3 3 '#102044'
RectPx 22 40 10 48 '#dff4ff'; RectPx 30 36 8 58 '#ffffff'; RectPx 60 35 14 60 '#dff4ff'
RectPx 72 47 8 38 '#b6e0ff'; RectPx 17 62 6 34 '#79bbf0'; RectPx 75 70 7 24 '#79bbf0'
RectPx 21 85 5 19 '#4f9fe3'; RectPx 29 88 6 15 '#8ed0ff'; RectPx 68 86 7 17 '#65aeea'; RectPx 76 82 4 14 '#102044'

# Cat ears.
RectPx 27 12 7 5 '#102044'; RectPx 34 13 10 6 '#ffffff'; RectPx 23 17 8 16 '#ffffff'
RectPx 31 18 13 10 '#eaf7ff'; RectPx 24 20 4 12 '#102044'; RectPx 27 21 10 13 '#ffb7c2'
RectPx 29 23 7 8 '#ff8497'; RectPx 34 27 6 3 '#d75070'
RectPx 63 13 11 6 '#102044'; RectPx 52 18 12 9 '#eaf7ff'; RectPx 69 18 8 17 '#ffffff'
RectPx 75 21 4 12 '#102044'; RectPx 65 22 9 12 '#ffb7c2'; RectPx 66 24 7 8 '#ff8497'
RectPx 59 27 7 3 '#d75070'

# Head, hair cap, and ahoge.
RectPx 34 17 28 5 '#ffffff'; RectPx 28 22 40 10 '#f8fdff'; RectPx 25 30 47 13 '#ffffff'
RectPx 23 40 51 14 '#ffffff'; RectPx 25 53 45 11 '#eef9ff'
RectPx 24 29 3 22 '#102044'; RectPx 70 30 3 23 '#102044'; RectPx 27 21 3 8 '#102044'; RectPx 66 21 4 8 '#102044'
RectPx 50 3 8 3 '#102044'; RectPx 47 6 10 4 '#ffffff'; RectPx 45 10 7 5 '#ffffff'
RectPx 44 15 5 6 '#8ecfff'; RectPx 49 6 7 3 '#102044'; RectPx 54 10 4 2 '#102044'
LinePx 37 26 29 51 '#8ed0ff' 2; LinePx 47 24 39 57 '#8ed0ff' 2
LinePx 58 25 67 55 '#8ed0ff' 2; LinePx 70 52 76 80 '#5da9e8' 2
LinePx 24 58 21 82 '#102044' 2; LinePx 72 58 76 78 '#102044' 2
LinePx 31 62 26 91 '#5da9e8' 2; LinePx 65 64 73 91 '#5da9e8' 2

# Face, bangs, eyes.
RectPx 31 45 34 25 '#ffe8d2'; RectPx 29 51 38 15 '#fff0dc'; RectPx 34 68 26 6 '#ffd6bd'
RectPx 31 45 2 20 '#102044'; RectPx 64 45 2 20 '#102044'
RectPx 35 31 6 25 '#ffffff'; RectPx 41 29 6 32 '#f8fdff'; RectPx 47 28 5 35 '#ffffff'
RectPx 52 30 6 28 '#f8fdff'; RectPx 58 34 5 20 '#e8f8ff'
LinePx 37 34 37 59 '#8ed0ff' 1; LinePx 47 32 47 61 '#8ed0ff' 1; LinePx 55 34 56 57 '#8ed0ff' 1
RectPx 33 50 9 10 '#20130c'; RectPx 54 50 9 10 '#20130c'
RectPx 35 57 7 6 '#a65f04'; RectPx 56 57 7 6 '#a65f04'
RectPx 36 59 6 3 '#ffc044'; RectPx 57 59 6 3 '#ffc044'
RectPx 37 51 3 3 '#ffffff'; RectPx 58 51 3 3 '#ffffff'; RectPx 41 53 2 3 '#d6cbff'; RectPx 62 53 2 3 '#d6cbff'
RectPx 28 62 5 2 '#ff8e96'; RectPx 64 62 5 2 '#ff8e96'; RectPx 47 63 2 2 '#ff6969'; RectPx 50 63 2 2 '#ff6969'

# Robe, bow, sleeves, wand, charm, legs.
RectPx 33 73 31 31 '#ffffff'; RectPx 29 78 12 21 '#f9f6ff'; RectPx 59 77 13 22 '#f9f6ff'
RectPx 35 101 28 4 '#102044'; RectPx 31 98 5 5 '#d7d5e4'; RectPx 64 96 5 6 '#d7d5e4'; RectPx 42 83 2 19 '#e2dfef'
RectPx 21 74 11 17 '#ffffff'; RectPx 18 85 11 9 '#f4f0ff'; RectPx 22 91 6 5 '#ffd7bd'
RectPx 68 75 12 18 '#ffffff'; RectPx 70 92 7 5 '#ffd7bd'; RectPx 20 93 7 3 '#914f15'; RectPx 70 96 6 3 '#914f15'
RectPx 42 69 5 6 '#ff8da1'; RectPx 50 69 5 6 '#ff8da1'; RectPx 45 72 7 5 '#e85f7c'
RectPx 37 72 8 10 '#ffb0bd'; RectPx 53 72 9 10 '#ffb0bd'; RectPx 41 81 6 9 '#ff8da1'; RectPx 53 81 6 9 '#ff8da1'
RectPx 45 76 8 4 '#c74666'; LinePx 39 73 46 78 '#7a253d' 1; LinePx 60 73 52 78 '#7a253d' 1
LinePx 17 53 25 88 '#102044' 2; RectPx 14 48 4 5 '#ff8da1'; RectPx 16 50 2 2 '#ffffff'
RectPx 18 63 6 15 '#ffffff'; RectPx 17 74 8 3 '#ff8da1'; RectPx 21 77 3 5 '#d75070'
RectPx 14 79 5 5 '#ffb0bd'; RectPx 20 82 4 5 '#ffffff'; RectPx 24 87 5 4 '#914f15'
LinePx 61 81 67 89 '#7a253d' 1; RectPx 66 88 9 15 '#b97832'; RectPx 68 90 5 3 '#ffd28a'
RectPx 68 95 2 2 '#ffffff'; RectPx 72 95 2 2 '#ffffff'; RectPx 70 98 2 2 '#ffffff'
RectPx 71 104 2 8 '#ff8da1'; RectPx 69 112 6 5 '#ffb0bd'
RectPx 39 104 8 13 '#ffe0c7'; RectPx 53 104 8 13 '#ffe0c7'; RectPx 37 116 11 8 '#ffffff'
RectPx 52 116 11 8 '#ffffff'; RectPx 37 123 11 3 '#ff8da1'; RectPx 52 123 11 3 '#ff8da1'
RectPx 38 113 10 2 '#ffd0bd'; RectPx 53 113 10 2 '#ffd0bd'
LinePx 32 73 31 102 '#102044' 1; LinePx 64 73 65 101 '#102044'
LinePx 33 73 22 76 '#102044'; LinePx 64 74 78 77 '#102044'
LinePx 37 124 48 124 '#102044'; LinePx 52 124 63 124 '#102044'

$out = Join-Path (Get-Location) 'catgirl_pixel_transparent_96.png'
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)

$scale = 4
$big = New-Object System.Drawing.Bitmap -ArgumentList ($w * $scale), ($h * $scale), $fmt
$gg = [System.Drawing.Graphics]::FromImage($big)
$gg.Clear([System.Drawing.Color]::Transparent)
$gg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$gg.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$gg.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$gg.DrawImage($bmp, 0, 0, $w * $scale, $h * $scale)
$gg.Dispose()

$out2 = Join-Path (Get-Location) 'catgirl_pixel_transparent_4x.png'
$big.Save($out2, [System.Drawing.Imaging.ImageFormat]::Png)

$g.Dispose()
$bmp.Dispose()
$big.Dispose()

Write-Output $out
Write-Output $out2
