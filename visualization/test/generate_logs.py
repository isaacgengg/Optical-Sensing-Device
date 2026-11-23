#!/usr/bin/env python3
"""Capture the current firmware log format from synthetic host samples."""
from pathlib import Path
import json
import os
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[2]
fw = repo / "firmware"
test = repo / "visualization/test"
with tempfile.TemporaryDirectory(prefix="optical-log-fixtures-") as tmp:
    binary = Path(tmp) / "logs"
    includes = [test / "shim", fw / "test/host/shim", fw / "core/domain/include",
                fw / "core/ports/include", fw / "adapters/log_sink/include", fw / "main"]
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                    *["-I" + str(p) for p in includes], str(test / "export_logs.c"),
                    str(fw / "core/domain/strip_analysis.c"),
                    str(fw / "adapters/log_sink/log_sink.c"), "-lm", "-o", str(binary)], check=True)
    output = subprocess.check_output([str(binary)], text=True)
fixtures = []
for block in output.split("CASE ")[1:]:
    name, log = block.split("\n", 1)
    fixtures.append({"name": name, "log": log})
destination = test / "generated"
destination.mkdir(exist_ok=True)
(destination / "logs.json").write_text(json.dumps(fixtures, indent=2) + "\n")
print(f"Generated {len(fixtures)} synthetic logs through the unchanged firmware log sink.")
