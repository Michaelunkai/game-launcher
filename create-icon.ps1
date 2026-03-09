# Create a gaming icon using .NET System.Drawing
Add-Type -AssemblyName System.Drawing

$size = 256
$bitmap = New-Object System.Drawing.Bitmap($size, $size)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias

# Background gradient (dark blue to purple)
$brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Point(0, 0)),
    (New-Object System.Drawing.Point($size, $size)),
    [System.Drawing.Color]::FromArgb(255, 30, 30, 80),
    [System.Drawing.Color]::FromArgb(255, 80, 30, 120)
)
$graphics.FillEllipse($brush, 0, 0, $size, $size)

# Controller shape (simplified)
$pen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 12)
$pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round

# D-pad
$graphics.DrawLine($pen, 70, 100, 110, 100)
$graphics.DrawLine($pen, 90, 80, 90, 120)

# Buttons (ABXY style)
$buttonBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$graphics.FillEllipse($buttonBrush, 160, 80, 25, 25)
$graphics.FillEllipse($buttonBrush, 185, 105, 25, 25)
$graphics.FillEllipse($buttonBrush, 135, 105, 25, 25)
$graphics.FillEllipse($buttonBrush, 160, 130, 25, 25)

# Play button symbol
$playPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 100, 255, 100), 20)
$playPoints = @(
    (New-Object System.Drawing.Point(100, 160)),
    (New-Object System.Drawing.Point(160, 128)),
    (New-Object System.Drawing.Point(100, 96))
)
$graphics.DrawPolygon($playPen, $playPoints)
$playBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 100, 255, 100))
$graphics.FillPolygon($playBrush, $playPoints)

# Save as PNG first
$pngPath = "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher\icon.png"
$bitmap.Save($pngPath, [System.Drawing.Imaging.ImageFormat]::Png)

Write-Host "Icon created at: $pngPath"

# Convert PNG to ICO using ImageMagick if available, otherwise use online converter
if (Get-Command magick -ErrorAction SilentlyContinue) {
    $icoPath = "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher\icon.ico"
    magick convert $pngPath -define icon:auto-resize=256,128,64,48,32,16 $icoPath
    Write-Host "ICO created at: $icoPath"
} else {
    Write-Host "ImageMagick not found. Will use PNG and convert during compilation."
    # Create a simple ICO manually
    $icoPath = "F:\study\Dev_Toolchain\programming\C++\projects\game-launcher\icon.ico"
    
    # Simple ICO header (single 256x256 image)
    $ico = [System.IO.File]::Create($icoPath)
    $writer = New-Object System.IO.BinaryWriter($ico)
    
    # ICO header
    $writer.Write([UInt16]0)  # Reserved
    $writer.Write([UInt16]1)  # Type (1 = ICO)
    $writer.Write([UInt16]1)  # Number of images
    
    # Image directory entry
    $writer.Write([byte]0)    # Width (0 = 256)
    $writer.Write([byte]0)    # Height (0 = 256)
    $writer.Write([byte]0)    # Colors
    $writer.Write([byte]0)    # Reserved
    $writer.Write([UInt16]1)  # Color planes
    $writer.Write([UInt16]32) # Bits per pixel
    
    $pngBytes = [System.IO.File]::ReadAllBytes($pngPath)
    $writer.Write([UInt32]$pngBytes.Length)  # Image size
    $writer.Write([UInt32]22)  # Image offset
    
    # Write PNG data
    $writer.Write($pngBytes)
    
    $writer.Close()
    $ico.Close()
    
    Write-Host "ICO created at: $icoPath"
}

$graphics.Dispose()
$bitmap.Dispose()
