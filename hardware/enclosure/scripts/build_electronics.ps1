# Lays out Electronics Assembly.iam and replaces stale routed wires with provisional pin-net markers.
# The actual underside wiring is not visible in the photos.
# Usage (Windows PowerShell, Inventor open with this repo's project active, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\build_electronics.ps1
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot "load_cad_types.ps1")
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$wiring = Join-Path $root "Electronics\Wiring.ipt"

[Electronics]::Run($inv, $root)

# Replace the wiring part (Electronics.Run has already removed it from the assembly).
foreach ($d in @($inv.Documents)) { if ($d.FullFileName -eq $wiring) { $d.Close($true) } }
if (Test-Path $wiring) { Remove-Item $wiring -Force }
[Wiring]::Build($inv, $wiring)
[Wiring]::Check($inv, $wiring)
$asm = $inv.Documents.Open((Join-Path $root "Electronics Assembly.iam"), $true)
$occ = $asm.ComponentDefinition.Occurrences.Add($wiring, $inv.TransientGeometry.CreateMatrix())
$occ.Grounded = $true
$asm.Update(); $asm.Save()
"Pin-net markers added; wire routing and continuity are not modeled"
