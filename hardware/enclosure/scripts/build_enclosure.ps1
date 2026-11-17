# Regenerates the enclosure parts from Enclosure.cs in a running Inventor 2027, makes the pouch slide
# (PouchSlide.cs) and adds the sliding LFA strip (LfaStrip.cs), checking both clear the enclosure over their travel.
# Usage (Windows PowerShell, Inventor open with this repo's project active, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\build_enclosure.ps1
# It refuses to overwrite existing files: close them in Inventor and delete Enclosure\Enclosure Base.ipt,
# Enclosure\Optical Module.ipt, Enclosure\Enclosure Lid.ipt and Enclosure Assembly.iam first (and
# Purchased Parts\DetectaChem\MobileDetect Pouch.ipt to regenerate the pouch).
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot "load_cad_types.ps1")
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
[Enclosure]::Build($inv, $root)
[PouchSlide]::Run($inv)
[LfaStrip]::AddTo($inv, $root)
