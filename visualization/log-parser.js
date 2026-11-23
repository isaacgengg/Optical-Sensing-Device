/* Parse the existing serial text. Never analyze spectra or compute a verdict. */
(function (root) {
  "use strict";
  const number = "[-+]?(?:(?:\\d+\\.?\\d*|\\.\\d+)(?:e[-+]?\\d+)?|inf|nan)";
  const numeric = new RegExp(`^${number}$`, "i");
  const summary = new RegExp(`^mode=(LATERAL_FLOW|COLORIMETRIC) verdict=(NEGATIVE|POSITIVE|INVALID) match=(\\S+) color=(\\S+) score=(${number}) control=(${number}) samples=(\\d+)\\s*$`, "i");
  const clean = text => text.replace(/\x1b\[[0-?]*[ -/]*[@-~]/g, "").replace(/\r$/, "");

  class LogParser {
    constructor() {
      this.buffer = "";
      this.current = null;
      this.source = "not reported";
      this.nextId = 0;
    }
    feed(chunk) {
      this.buffer += chunk;
      const lines = this.buffer.split("\n");
      this.buffer = lines.pop();
      const events = lines.flatMap(line => this.parseLine(clean(line)));
      if (this.buffer.length > 65536) {
        this.buffer = "";
        this.current = null;
        events.push({type:"warning", message:"Discarded an oversized incomplete log line."});
      }
      return events;
    }
    finish() {
      const tail = this.buffer;
      this.buffer = "";
      return tail ? this.parseLine(clean(tail)) : [];
    }
    snapshot() {
      return {...this.current, log_lines:[...this.current.log_lines]};
    }
    parseLine(line) {
      const events = [{type:"line", line}];
      const uptime = line.match(/^[IWEVD]\s*\((\d+)\)/)?.[1] ?? null;
      // A boot/source line is a session boundary: never attach its spectra to
      // a result from before a reboot. Absence of this line means unknown source.
      if (/\b(?:rst:0x|ESP-ROM:)/.test(line)) {
        this.current = null;
        this.source = "not reported";
      }
      const source = line.match(/\bmain:\s+source=(hardware|MOCK)\b/);
      if (source) { this.source = source[1]; this.current = null; }
      const error = line.match(/\bmain:\s+(measurement failed:\s*.+)/);
      if (error) {
        this.current = null;
        events.push({type:"error", message:error[1], uptime_ms:uptime, hardware_source:this.source, line});
        return events;
      }
      const message = line.match(/\blog_sink:\s*(.*)$/)?.[1];
      if (message == null) return events;
      if (message.startsWith("mode=")) {
        this.current = null;
        const match = message.match(summary);
        if (!match) {
          events.push({type:"warning", message:"Unrecognized result line; preserved in the log below."});
          return events;
        }
        this.current = {
          id:++this.nextId, mode:match[1], verdict:match[2], match:match[3], color:match[4],
          score:match[5], control:match[6], samples:match[7], uptime_ms:uptime,
          hardware_source:this.source, calibration:null, metric:null, zones:null,
          raw:null, test_raw:null, control_raw:null, refl:null, log_lines:[line]
        };
        events.push({type:"result", result:this.snapshot()});
        return events;
      }
      if (!this.current) return events;
      const zones = message.match(/^zones:\s*(.*);\s*metric=(\S+)\s+calibration=(\S+)\s*$/);
      if (zones && this.current.mode === "LATERAL_FLOW" && this.current.zones == null) {
        Object.assign(this.current, {zones:zones[1], metric:zones[2], calibration:zones[3]});
      } else {
        const channels = message.match(/^(raw|test_raw|control_raw|refl)=(.*)$/);
        if (!channels) return events;
        const [, key, text] = channels;
        const allowed = this.current.mode === "COLORIMETRIC" ? ["raw", "refl"] : ["test_raw", "control_raw"];
        if (!allowed.includes(key) || this.current[key] != null) return events;
        const values = text.trim().replace(/,\s*$/, "").split(",").map(v => v.trim());
        if (!values.length || values.length > 16 || !values.every(v => numeric.test(v))) {
          events.push({type:"warning", message:`Unrecognized ${key} values; preserved in the log below.`});
          return events;
        }
        this.current[key] = values;
      }
      this.current.log_lines.push(line);
      events.push({type:"update", result:this.snapshot()});
      return events;
    }
  }
  root.OpticalLog = {LogParser};
  if (typeof module !== "undefined" && module.exports) module.exports = {LogParser};
})(globalThis);
