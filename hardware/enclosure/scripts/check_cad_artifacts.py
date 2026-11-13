"""Fast CI freshness check of committed CAD and its recorded verification.

Does not rerun the geometry kernel or certify the physical device. Run
build_as_built.py for fresh solids, interference checks and neutral exports.
"""
import hashlib
import json

from layout import CONFIG, ROOT, csharp, dimensions


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    c,d = dimensions()
    assert (ROOT/"scripts/AsBuiltLayout.cs").read_text() == csharp(d), "Stale C# dimensions"
    out = ROOT/"exports/as-built"
    r = json.loads((out/"validation.json").read_text())
    assert r["layout_sha256"] == digest(CONFIG), "Stale layout report"
    for path,sha in r["source_sha256"].items():
        assert digest(ROOT/path) == sha, f"CAD source changed; regenerate: {path}"
    for path,sha in r["sha256"].items():
        assert digest(out/path) == sha, f"CAD output differs from validated file: {path}"
    for destination,source in r["compatibility_copies"].items():
        assert digest(ROOT/destination) == digest(out/source), f"Stale compatibility export: {destination}"
    required = {"Enclosure Base","Enclosure Lid","Test Sled","Alignment Gauge"}
    required.update(f"Optical Module d{ap:.1f}" for ap in c["apertures_mm"])
    assert set(r["parts"]) == required
    for name,part in r["parts"].items():
        assert part["valid"] and part["solids"] == 1 and part["volume_mm3"]>0, name
        assert part["nonmanifold_edges"] == 0 and part["step_volume_error_mm3"]<1e-3, name
    for name,checks in r["ray_checks"].items():
        assert set(checks) == {"TCS_test","AS_control","TCS_color"}, name
        for v in checks.values():
            assert v["clear_led_sample_rays"] > 0 and v["view_axis_blocked_mm"] < 1e-5, name
    for name,checks in r["optics"].items():
        for v in checks:
            assert v["minimum_bore_gap_mm"] >= .4 and v["bore_overlap_mm3"] < 1e-6, name
    for fixture in ("Enclosure Base","Test Sled"):
        for sample,positions in [("strip",81),("pouch",96)]:
            check = r["fit"][f"{fixture}/{sample}"]
            assert check["positions"] == positions and check["max_overlap_mm3"] < 1e-4
    print(f"CAD artifacts current: {len(required)} printable parts, {len(r['sha256'])} neutral files; recorded geometry checks passed.")


if __name__ == "__main__":
    main()
