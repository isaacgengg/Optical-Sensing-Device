# Windows + Inventor 2027. Builds in a NEW directory; never overwrites source .iam/.ipt.
# Close all Inventor documents, start a fresh PowerShell process, and run this file.
# This native workflow has not been executed on the Mac that generated the STEP/STL prototype.
param([string]$Destination = (Join-Path $PSScriptRoot "..\exports\as-built-native"))
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot "load_cad_types.ps1")
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
if ($inv.Documents.Count -ne 0) { throw "Save and close all Inventor documents before rebuilding in the isolated project." }
if (Test-Path $Destination) { throw "Destination already exists. Choose a new -Destination; existing work will not be replaced." }
$source = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
New-Item -ItemType Directory $Destination | Out-Null
$root = (Resolve-Path $Destination).Path
foreach ($name in @("Electronics", "Purchased Parts", "Electronics Assembly.iam", "Optical-Sensing-Device.ipj")) {
    Copy-Item (Join-Path $source $name) $root -Recurse
}
$project = $inv.DesignProjectManager.DesignProjects.AddExisting((Join-Path $root "Optical-Sensing-Device.ipj"))
$project.Activate($false)
$log = New-Object System.Collections.Generic.List[string]
$log.Add([Electronics]::Run($inv, $root))
$wiring = Join-Path $root "Electronics\Wiring.ipt"
foreach ($doc in @($inv.Documents)) { if ($doc.FullFileName -eq $wiring) { $doc.Close($true) } }
if (Test-Path $wiring) { Remove-Item $wiring } # disposable copy in the new build directory
$log.Add([Wiring]::Build($inv, $wiring))
$asm = $inv.Documents.Open((Join-Path $root "Electronics Assembly.iam"), $true)
$occ = $asm.ComponentDefinition.Occurrences.Add($wiring, $inv.TransientGeometry.CreateMatrix())
$occ.Name = "Wiring (pin net markers only)"; $occ.Grounded = $true
$asm.Update(); $asm.Save()
$log.Add([Enclosure]::Build($inv, $root))
$log.Add([PouchSlide]::Run($inv))
$log.Add([LfaStrip]::AddTo($inv, $root))
# Use one sample at a time. The pouch remains available but is hidden in the LFA view.
foreach ($o in $inv.ActiveDocument.ComponentDefinition.Occurrences) {
    if ($o.Name.StartsWith("MobileDetect Pouch")) { $o.Visible = $false }
}
$inv.ActiveDocument.Save()
$log.Add([Enclosure]::BuildJig($inv, $root, [double[]]@(1.2,1.6,2.0,2.5)))
$log.Add([PouchSlide]::Run($inv))
$log.Add([LfaStrip]::AddTo($inv, $root))
foreach ($o in $inv.ActiveDocument.ComponentDefinition.Occurrences) {
    if ($o.Name.StartsWith("MobileDetect Pouch")) { $o.Visible = $false }
}
$inv.ActiveDocument.Save()
$log.Add([LfaStrip]::Run($inv, $root))
$log | Set-Content (Join-Path $root "native-build-log.txt")
Write-Host "Native source regenerated in $root. Review the saved log, fit, references and appearance before committing."
