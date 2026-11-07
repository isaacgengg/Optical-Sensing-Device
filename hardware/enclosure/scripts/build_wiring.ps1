# Regenerates Electronics/Wiring.ipt from Wiring.cs in a running Inventor 2027, checks that wires of
# different nets don't overlap, and adds it to Electronics Assembly.iam.
# Delete the old Wiring.ipt (and remove it from the assembly) first; this refuses to overwrite.
$ErrorActionPreference = 'Stop'
$dll = "C:\Program Files\Autodesk\Inventor 2027\Bin\Autodesk.Inventor.Interop.dll"
Add-Type -Path $dll
Add-Type -TypeDefinition (Get-Content (Join-Path $PSScriptRoot "Wiring.cs") -Raw) -ReferencedAssemblies $dll
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$wiring = Join-Path $root "Electronics\Wiring.ipt"
[Wiring]::Build($inv, $wiring)
[Wiring]::Check($inv, $wiring)
$asm = $inv.Documents.Open((Join-Path $root "Electronics Assembly.iam"), $true)
$occ = $asm.ComponentDefinition.Occurrences.Add($wiring, $inv.TransientGeometry.CreateMatrix())
$occ.Grounded = $true
$asm.Update(); $asm.Save()
"Added to Electronics Assembly"
