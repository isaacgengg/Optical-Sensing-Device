# Makes the pouch in Enclosure Assembly slide along the tunnel (see PouchSlide.cs).
# Run once on a freshly generated Enclosure Assembly.iam, with it open and active in Inventor.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot "load_cad_types.ps1")
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
[PouchSlide]::Run($inv)
