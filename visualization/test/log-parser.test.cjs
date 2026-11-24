const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const {LogParser} = require("../log-parser.js");
const fixtures = JSON.parse(fs.readFileSync(path.join(__dirname,"generated/logs.json")));
const parse = text => {const p=new LogParser();return [...p.feed(text),...p.finish()];};
const last = events => events.filter(e=>e.result).at(-1)?.result;
let passed=0;
function check(name,fn){fn();passed++;console.log(`PASS ${name}`);}
for(const fixture of fixtures.filter(f=>f.name!=="lfa_dark"))check(`Firmware C log -> UI data: ${fixture.name}`,()=>{
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
  const diagnostic=fixture.log.match(/color_match: (.*)/)?.[1];
  if(diagnostic){
    for(const [field,key] of [["match_status","status"],["nearest_color","nearest"],["runner_up_color","runner_up"],
      ["runner_up_score","runner_up_score"],["color_margin","margin"],["max_distance","max_distance"],["min_margin","min_margin"]])
      assert.equal(r[field],diagnostic.match(new RegExp(`(?:^| )${key}=(\\S+)`))[1]);
    assert(r.log_lines.some(line=>line.includes("color_match:")));
  }
});
check("Dark captures preserve fractional counts without producing assay results",()=>{
  const dark=fixtures.find(f=>f.name==="lfa_dark").log;
  assert(dark.includes("capture=LFA_DARK illumination=off samples=5 test_gain=128 test_ms=100 control_gain=64 control_ms=100"));
  assert(dark.includes("dark_test_raw=0.200, 0.400,"));
  assert(dark.includes("dark_control_raw=0.600, 0.800,"));
  assert(!parse(dark).some(e=>e.result));
  const p=new LogParser();
  const normal=fixtures.find(f=>f.name==="uncalibrated_lfa").log;
  p.feed(normal.split("\n").slice(0,2).join("\n")+"\n");
  const events=p.feed(dark.split("\n").slice(1).join("\n")+"I (200) log_sink: test_raw=1, 2,\n");
  assert(!events.some(e=>e.result));
  assert(events.some(e=>e.line?.includes("dark_test_raw=")));
  assert(last(p.feed(normal)));
});
check("All five colors retain their configured match labels",()=>{
  const labels={white:"control",yellow:"psychedelics",turquoise:"fentanyl",orange:"cocaine",deep_purple:"explosives"};
  for(const [color,match] of Object.entries(labels)){
    const r=last(parse(fixtures.find(f=>f.name===`${color}_full`).log));
    assert.equal(r.color,color);assert.equal(r.match,match);assert.equal(r.match_status,"matched");
  }
});
check("Ambiguous firmware results stay rejected with their diagnostics",()=>{
  const r=last(parse(fixtures.find(f=>f.name==="ambiguous_color").log));
  assert.equal(r.verdict,"INVALID");assert.equal(r.color,"none");assert.equal(r.match_status,"ambiguous");
  assert.equal(r.max_distance,"0.025");assert.equal(r.min_margin,"0.010");
  assert.deepEqual(new Set([r.nearest_color,r.runner_up_color]),new Set(["turquoise","deep_purple"]));
});
check("Diagnostics are copied even when their numbers contradict their status",()=>{
  const r=last(parse("log_sink: mode=COLORIMETRIC verdict=POSITIVE match=deep_purple color=deep_purple score=99.999 control=0.00 samples=5\n"+
    "log_sink: color_match: status=matched nearest=deep_purple runner_up=orange runner_up_score=0.000001 margin=-99.998999 max_distance=0.025 min_margin=0.010\n"));
  assert.equal(r.verdict,"POSITIVE");assert.equal(r.color,"deep_purple");assert.equal(r.match_status,"matched");
  assert.equal(r.color_margin,"-99.998999");
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
