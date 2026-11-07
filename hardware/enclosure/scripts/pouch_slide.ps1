# Makes the pouch in Enclosure Assembly slide along the tunnel (see PouchSlide.cs).
# Run once on a freshly generated Enclosure Assembly.iam, with it open and active in Inventor.
$ErrorActionPreference = 'Stop'
$dll = "C:\Program Files\Autodesk\Inventor 2027\Bin\Autodesk.Inventor.Interop.dll"
Add-Type -Path $dll
Add-Type -TypeDefinition (Get-Content (Join-Path $PSScriptRoot "PouchSlide.cs") -Raw) -ReferencedAssemblies $dll
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
[PouchSlide]::Run($inv)
