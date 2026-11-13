# Compile the shared layout and all generators into one assembly. Use a fresh
# Windows PowerShell process after editing source; Add-Type cannot unload types.
$ErrorActionPreference = 'Stop'
$layoutHash = (Get-FileHash (Join-Path $PSScriptRoot "..\as-built-layout.json") -Algorithm SHA256).Hash.ToLower()
$layoutSource = Get-Content (Join-Path $PSScriptRoot "AsBuiltLayout.cs") -Raw
if (-not $layoutSource.Contains('LayoutSha256 = "' + $layoutHash + '"')) {
    throw "AsBuiltLayout.cs is stale. Run python scripts/layout.py before the native build."
}
$dll = "C:\Program Files\Autodesk\Inventor 2027\Bin\Autodesk.Inventor.Interop.dll"
if (-not ("Enclosure" -as [type])) {
    Add-Type -Path $dll
    $sources = @("AsBuiltLayout.cs", "Electronics.cs", "Wiring.cs", "Enclosure.cs", "PouchSlide.cs", "LfaStrip.cs") |
        ForEach-Object { Join-Path $PSScriptRoot $_ }
    Add-Type -Path $sources -ReferencedAssemblies $dll, "System.Core"
}
