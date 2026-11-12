# Lays out Electronics Assembly.iam (Electronics.cs), then regenerates the wiring (Wiring.cs), checks that wires
# of different nets don't overlap, and adds it back to the assembly.
# Usage (Windows PowerShell, Inventor open with this repo's project active, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\build_electronics.ps1
$ErrorActionPreference = 'Stop'
$dll = "C:\Program Files\Autodesk\Inventor 2027\Bin\Autodesk.Inventor.Interop.dll"
Add-Type -Path $dll
Add-Type -TypeDefinition (Get-Content (Join-Path $PSScriptRoot "Electronics.cs") -Raw) -ReferencedAssemblies $dll
Add-Type -TypeDefinition (Get-Content (Join-Path $PSScriptRoot "Wiring.cs") -Raw) -ReferencedAssemblies $dll
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
"Wiring added to Electronics Assembly"
