# Exports the printable parts of the optics test jig (Test Jig\*.ipt) as STEP and STL into exports\test-jig\.
# STEP for slicers that take it (PrusaSlicer, Bambu Studio, OrcaSlicer); STL for the rest (Cura, etc.).
# Both are in millimetres and keep the assembly's coordinates, so rotate parts on the build plate as the README
# describes. STLs use a fine mesh (0.001 mm surface deviation, 3 deg normal deviation) so the small apertures
# stay round.
# Usage (Windows PowerShell, Inventor open with this repo's project active, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\export_test_jig.ps1
$ErrorActionPreference = 'Stop'
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$src = Join-Path $root "Test Jig"
$out = Join-Path $root "exports\test-jig"
New-Item -ItemType Directory -Force $out | Out-Null

$stl = $inv.ApplicationAddIns.ItemById("{533E9A98-FC3B-11D4-8E7E-0010B541CD80}")   # Inventor STL translator
if (-not $stl.Activated) { $stl.Activate() }
$ctx = $inv.TransientObjects.CreateTranslationContext(); $ctx.Type = 13059          # kFileBrowseIOMechanism

foreach ($f in Get-ChildItem $src -Filter *.ipt) {
    $doc = $inv.Documents.Open($f.FullName, $false)

    $step = Join-Path $out ($f.BaseName + ".step")
    if (Test-Path $step) { Remove-Item $step -Force }
    $doc.SaveAs($step, $true)                 # copy; STEP AP214 is picked from the extension

    $opt = $inv.TransientObjects.CreateNameValueMap()
    [void]$stl.HasSaveCopyAsOptions($doc, $ctx, $opt)
    $opt.Value("ExportUnits") = 5             # millimetres
    $opt.Value("Resolution") = 3              # custom
    $opt.Value("SurfaceDeviation") = 0.001
    $opt.Value("NormalDeviation") = 3
    $opt.Value("MaxEdgeLength") = 100
    $opt.Value("AspectRatio") = 21.5
    $opt.Value("OutputFileType") = 0          # binary
    $data = $inv.TransientObjects.CreateDataMedium(); $data.FileName = Join-Path $out ($f.BaseName + ".stl")
    $stl.SaveCopyAs($doc, $ctx, $opt, $data)

    $doc.Close($true)
    "{0,-26} STEP {1,5:N0} KB   STL {2,5:N0} KB" -f $f.BaseName, ((Get-Item $step).Length / 1KB), ((Get-Item $data.FileName).Length / 1KB)
}
