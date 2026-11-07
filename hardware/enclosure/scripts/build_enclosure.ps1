# Regenerates the enclosure parts from Enclosure.cs in a running Inventor 2027, then makes the pouch slide
# (PouchSlide.cs) and checks it clears the tunnel over its whole travel.
# Usage (Windows PowerShell, Inventor open, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\build_enclosure.ps1
# It refuses to overwrite existing files: close them in Inventor and delete Enclosure\Enclosure Base.ipt,
# Enclosure\Optical Module.ipt, Enclosure\Enclosure Lid.ipt and Enclosure Assembly.iam first.
$ErrorActionPreference = 'Stop'
$dll = "C:\Program Files\Autodesk\Inventor 2027\Bin\Autodesk.Inventor.Interop.dll"
Add-Type -Path $dll
Add-Type -TypeDefinition (Get-Content (Join-Path $PSScriptRoot "Enclosure.cs") -Raw) -ReferencedAssemblies $dll
Add-Type -TypeDefinition (Get-Content (Join-Path $PSScriptRoot "PouchSlide.cs") -Raw) -ReferencedAssemblies $dll, "System.Core"
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
[Enclosure]::Build($inv, (Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
[PouchSlide]::Run($inv)
