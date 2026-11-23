const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const {LogParser} = require("../log-parser.js");
const fixtures = JSON.parse(fs.readFileSync(path.join(__dirname,"generated/logs.json")));
const parse = text => {const p=new LogParser();return [...p.feed(text),...p.finish()];};
const last = events => events.filter(e=>e.result).at(-1)?.result;
let passed=0;
function check(name,fn){fn();passed++;console.log(`PASS ${name}`);}
for(const fixture of fixtures)check(`Unchanged C log -> UI data: ${fixture.name}`,()=>{
  const r=last(parse(fixture.log));
  assert(r);
  const summary=fixture.log.match(/mode=.*\n/)[0].trim();
  for(const key of ["mode","verdict","match","color","score","control","samples"])
    assert.equal(r[key],summary.match(new RegExp(`(?:^| )${key}=(\\S+)`))[1]);
  assert.equal(r.hardware_source,"MOCK");
  assert.equal(r.uptime_ms,"100");
  if(r.mode==="COLORIMETRIC"){
    assert.equal(r.raw.length,14);assert.equal(r.refl.length,14);
    assert.equal(r.calibration,null);
  }else{assert.equal(r.test_raw.length,14);assert.equal(r.control_raw.length,14);}
  for(const key of ["raw","refl","test_raw","control_raw"]){
    if(!r[key])continue;
    const exact=fixture.log.match(new RegExp(`log_sink: ${key}=(.*)`))[1].trim().replace(/,\s*$/,"").split(",").map(v=>v.trim());
    assert.deepEqual(r[key],exact);
  }
});
check("Every byte split, ANSI escapes and CRLF preserve output",()=>{
  const text=fixtures[4].log.replaceAll("I (","\x1b[0;32mI (").replaceAll("\n","\x1b[0m\r\n");
  const p=new LogParser();let events=[];
  for(const char of text)events.push(...p.feed(char));
  events.push(...p.finish());
  assert.deepEqual(last(events),last(parse(fixtures[4].log)));
});
check("Arbitrary partial chunks do not emit unfinished summaries",()=>{
  const p=new LogParser();assert.equal(p.feed("I (1) log_sink: mode=COLOR").length,0);
  const e=p.feed("IMETRIC verdict=POSITIVE match=fentanyl color=red score=99.999 control=0.00 samples=5\n");
  assert.equal(last(e).score,"99.999");assert.equal(last(e).verdict,"POSITIVE");
});
check("Display input is copied even when scores disagree with current thresholds",()=>{
  const r=last(parse("log_sink: mode=COLORIMETRIC verdict=NEGATIVE match=custom color=purple score=123.456 control=8.70 samples=9"));
  assert.equal(r.verdict,"NEGATIVE");assert.equal(r.color,"purple");assert.equal(r.match,"custom");
  assert.equal(r.score,"123.456");assert.equal(r.control,"8.70");assert.equal(r.hardware_source,"not reported");
});
check("Missing LFA calibration keeps literal zero scores",()=>{
  const r=last(parse(fixtures.find(f=>f.name==="uncalibrated_lfa").log));
  assert.equal(r.calibration,"missing_or_invalid");assert.equal(r.verdict,"INVALID");
  assert.equal(r.score,"0.000");assert.equal(r.control,"0.00");
});
check("Malformed summary cannot attach channels to a prior result",()=>{
  const p=new LogParser();p.feed(fixtures[0].log);
  const e=p.feed("I (200) log_sink: mode=COLORIMETRIC broken\nI (200) log_sink: raw=1, 2,\n");
  assert(e.some(e=>e.type==="warning"));assert(!e.some(e=>e.result));
});
check("Errors are log events, never invented assay verdicts",()=>{
  const p=new LogParser();p.feed(fixtures[0].log);
  const e=p.feed("W (234) main: measurement failed: ESP_ERR_INVALID_RESPONSE\nI (234) log_sink: raw=2, 3,\n");
  assert.equal(e.find(e=>e.type==="error").message,"measurement failed: ESP_ERR_INVALID_RESPONSE");
  assert(!e.some(e=>e.result));
});
check("Boot boundaries clear incomplete readings and old source metadata",()=>{
  const p=new LogParser();p.feed(fixtures[0].log);
  const e=p.feed("rst:0x1\nI (10) log_sink: raw=1, 2,\nlog_sink: mode=COLORIMETRIC verdict=INVALID match=none color=none score=0.000 control=0.00 samples=5\n");
  assert.equal(last(e).raw,null);assert.equal(last(e).hardware_source,"not reported");
});
check("Unrelated warnings and orphan spectra cannot create results",()=>{
  const e=parse("W (12) as7343: CFG20=0x02: auto smux not 18-channel, rewriting\nI (20) log_sink: control_raw=1, 2,\n");
  assert(!e.some(e=>e.result));
});
check("Unknown fields stay absent, including truncated captures",()=>{
  const r=last(parse(fixtures[0].log.split("\n").slice(0,2).join("\n")));
  assert.equal(r.raw,null);assert.equal(r.refl,null);assert.equal(r.calibration,null);
});
check("Non-finite tokens remain visible rather than being silently replaced",()=>{
  const r=last(parse("log_sink: mode=COLORIMETRIC verdict=INVALID match=none color=none score=inf control=nan samples=5\n"));
  assert.equal(r.score,"inf");assert.equal(r.control,"nan");
});
check("Bad vector tokens are kept in console, not interpreted",()=>{
  const e=parse(fixtures[0].log.split("\n").slice(0,2).join("\n")+"\nlog_sink: raw=1, bad, 3,\n");
  assert(e.some(e=>e.type==="warning"));assert.equal(last(e).raw,null);
});
console.log(`${passed} log-parser checks passed`);
