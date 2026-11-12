# Exports Enclosure Assembly and Electronics Assembly to STEP (AP214) next to the .iam files,
# so the model can be viewed without Inventor (https://3dviewer.net, FreeCAD).
# Run in Windows PowerShell with Inventor open and Optical-Sensing-Device.ipj as the active project.
# Re-run after changing the model. The pouch is exported at its current Insertion position.
$ErrorActionPreference = 'Stop'
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
$step = $inv.ApplicationAddIns.ItemById("{90AF7F40-0C01-11D5-8E83-0010B541CD80}")  # "Translator: STEP"
if (-not $step.Activated) { $step.Activate() }
$root = Split-Path $PSScriptRoot
foreach ($name in "Enclosure Assembly", "Electronics Assembly") {
    $path = Join-Path $root "$name.iam"
    $wasOpen = @($inv.Documents | Where-Object { $_.FullFileName -eq $path }).Count -gt 0
    $doc = $inv.Documents.Open($path, $false)
    $ctx = $inv.TransientObjects.CreateTranslationContext()
    $ctx.Type = 13059  # kFileBrowseIOMechanism
    $opts = $inv.TransientObjects.CreateNameValueMap()
    $null = $step.HasSaveCopyAsOptions($doc, $ctx, $opts)  # fills in the defaults (AP214)
    $out = $inv.TransientObjects.CreateDataMedium()
    $out.FileName = Join-Path $root "$name.step"
    $step.SaveCopyAs($doc, $ctx, $opts, $out)
    if (-not $wasOpen) { $doc.Close($true) }
    Write-Host "Wrote $($out.FileName)"
}
