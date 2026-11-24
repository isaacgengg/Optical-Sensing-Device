"use strict";
const byId = id => document.getElementById(id);
const records = [];
const recordByKey = new Map();
let sessionCount = 0;
let latestKey = null;
let serialPort = null;
let serialReader = null;
let serialTask = null;
let stopping = false;
let lastReceivedAt = null;
let writeTask = null;

function setStatus(text, live, note) {
  byId("status").textContent = text;
  byId("status").classList.toggle("live", live);
  byId("note").textContent = note;
}
function newSession(source) {
  return {id:++sessionCount, source, parser:new OpticalLog.LogParser()};
}
function renderRecord(record) {
  const r = record.result;
  byId("verdict").textContent = r?.verdict ?? "No verdict reported";
  byId("test-type").textContent = r?.mode ?? "Not reported";
  byId("matched-color").textContent = r?.color ?? "—";
  byId("reference").textContent = r?.match ?? "—";
  byId("match-status").textContent = r?.match_status ?? "Not reported";
  byId("nearest-color").textContent = r?.nearest_color ?? "Not reported";
  byId("runner-up-color").textContent = r?.runner_up_color ?? "Not reported";
  byId("color-margin").textContent = r?.color_margin ?? "—";
  byId("score").textContent = r?.score ?? "—";
  byId("control-score").textContent = r?.control ?? "—";
  byId("sample-count").textContent = r?.samples ?? "—";
  byId("calibration").textContent = r?.calibration ?? "Not reported in log";
  byId("measurement-id").textContent = (r?.uptime_ms ?? record.uptime_ms) != null ? `${r?.uptime_ms ?? record.uptime_ms} ms since device boot` : "Not reported";
  byId("result-source").textContent = `${record.source} · source=${r?.hardware_source ?? record.hardware_source ?? "not reported"}`;
  byId("result-note").textContent = record.error ?? (r?.calibration === "missing_or_invalid"
    ? "Firmware reports missing_or_invalid calibration. Displayed scores are the literal logged values."
    : "Values and labels are copied from the firmware log. The page does not calculate a result.");
  const spectra = byId("spectra");
  spectra.replaceChildren();
  const raw = r?.raw ?? r?.test_raw ?? [], control = r?.control_raw ?? [], refl = r?.refl ?? [];
  const count = Math.max(raw.length, control.length, refl.length);
  if (!count) {
    const row = spectra.insertRow(); const cell = row.insertCell(); cell.colSpan=4;
    cell.textContent="No channel values received for this result.";
  }
  for (let i=0;i<count;i++) {
    const row=spectra.insertRow();
    for (const value of [i, raw[i]??"—", control[i]??"—", refl[i]??"—"]) row.insertCell().textContent=value;
  }
  byId("result-log").textContent = r?.log_lines.join("\n") ?? record.error_line ?? "";
  updateFreshness();
}
function renderHistoryRow(record) {
  const r = record.result;
  record.row.replaceChildren();
  for (const value of [record.index, new Date(record.received_at).toLocaleTimeString(), record.source,
    r?.mode??"—", r?.color??"—", r?.verdict??"—", r?.score??"—", r?.control??"—", r?.samples??"—"]) {
    const cell=document.createElement("td");cell.textContent=value;record.row.append(cell);
  }
  if (record.error) record.row.title=record.error;
}
function acceptEvents(session, events) {
  const lines=[];
  for (const event of events) {
    if (event.type==="line") {lines.push(event.line);continue;}
    if (event.type==="warning") {byId("note").textContent=event.message;continue;}
    if (event.type==="result" || event.type==="error") {
      const key=`${session.id}:${event.type}:${event.result?.id ?? records.length}`;
      const record={key,index:records.length+1,source:session.source,received_at:new Date().toISOString(),
        result:event.result,error:event.message,error_line:event.line,hardware_source:event.hardware_source,uptime_ms:event.uptime_ms,row:document.createElement("tr")};
      records.push(record);recordByKey.set(key,record);latestKey=key;lastReceivedAt=Date.now();
      byId("empty")?.remove();byId("history").prepend(record.row);byId("export").disabled=false;
      renderHistoryRow(record);renderRecord(record);
    } else if (event.type==="update") {
      const key=`${session.id}:result:${event.result.id}`, record=recordByKey.get(key);
      if (!record) continue;
      record.result=event.result;
      if (latestKey===key) renderRecord(record);
    }
  }
  if (lines.length) {
    const log=byId("serial-log");
    // Keep the on-screen console bounded; parsed records/CSV remain available.
    log.textContent=(log.textContent+lines.join("\n")+"\n").split("\n").slice(-2000).join("\n");
    log.scrollTop=log.scrollHeight;
  }
}
function ingestLog(text, source="Pasted log") {
  const session=newSession(source);
  if (!serialPort) setStatus("Viewing log",false,`Loaded ${source.toLowerCase()}. These are recorded outputs, not a live connection.`);
  acceptEvents(session,session.parser.feed(text));
  acceptEvents(session,session.parser.finish());
}
function updateFreshness(now=Date.now()) {
  if(lastReceivedAt==null)return;
  const seconds=Math.max(0,Math.floor((now-lastReceivedAt)/1000));
  byId("freshness").textContent=`Displayed ${seconds}s ago · ${serialPort ? "USB connected" : "recorded output"}`;
}
setInterval(updateFreshness,1000);
function csvText() {
  const columns=["received_at","input_source","hardware_source","uptime_ms","mode","verdict","match","color","score","control","samples","calibration","metric","zones","match_status","nearest_color","runner_up_color","runner_up_score","color_margin","max_distance","min_margin","raw","test_raw","control_raw","refl","error"];
  const cell=value=>{
    let text=Array.isArray(value)?value.join(", "):String(value??"");
    // Preserve numbers; stop untrusted textual labels being treated as formulas.
    if (/^[=+@-]/.test(text) && !/^[-+]?\d+(?:\.\d+)?(?:e[-+]?\d+)?$/i.test(text)) text="'"+text;
    return '"'+text.replaceAll('"','""')+'"';
  };
  return [columns.join(","),...records.map(record=>columns.map(key=>cell(
    key==="received_at"?record.received_at:key==="input_source"?record.source:key==="error"?record.error:record.result?.[key]??record[key]
  )).join(","))].join("\r\n")+"\r\n";
}
byId("export").addEventListener("click",()=>{
  const url=URL.createObjectURL(new Blob([csvText()],{type:"text/csv;charset=utf-8"}));
  const link=document.createElement("a");link.href=url;link.download=`optical-log-${Date.now()}.csv`;link.click();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
});
byId("import").addEventListener("click",()=>ingestLog(byId("paste-log").value));
byId("log-file").addEventListener("change",async event=>{
  const file=event.target.files[0];if(!file)return;
  try {ingestLog(await file.text(),`File: ${file.name}`);} catch(err){byId("note").textContent=`Could not read file: ${err.message}`;}
  event.target.value="";
});
function setSerialControls(connected) {
  byId("connect").textContent=connected?"Disconnect USB":"Connect USB log";
  byId("measure-color").disabled=!connected;byId("measure-lfa").disabled=!connected;
  // Keep imported logs from replacing a live result while its extra lines arrive.
  byId("import").disabled=connected;byId("log-file").disabled=connected;
}
async function readSerial(port,session) {
  const decoder=new TextDecoder();
  let error=null;
  try {
    serialReader=port.readable.getReader();
    while(!stopping) {
      const {value,done}=await serialReader.read();
      if(done)break;
      acceptEvents(session,session.parser.feed(decoder.decode(value,{stream:true})));
    }
    acceptEvents(session,session.parser.feed(decoder.decode()));
    acceptEvents(session,session.parser.finish());
  } catch(err) {error=err;}
  finally {
    serialReader?.releaseLock();serialReader=null;
    if(writeTask)await writeTask.catch(()=>{});
    try {await port.close();} catch(err) {error??=err;}
    serialPort=null;setSerialControls(false);byId("connect").disabled=false;
    setStatus("Disconnected",false,error?`Serial connection ended: ${error.message}`:"USB log disconnected. Displayed readings remain in history.");
    updateFreshness();
  }
}
byId("connect").addEventListener("click",async()=>{
  byId("connect").disabled=true;
  try {
    if(serialPort) {stopping=true;await serialReader?.cancel();await serialTask;return;}
    const port=await navigator.serial.requestPort();
    await port.open({baudRate:115200});
    serialPort=port;stopping=false;setSerialControls(true);
    setStatus("USB connected",true,"Reading the existing log at 115200 baud. Color sends c; LFA sends l, just like the serial monitor.");
    serialTask=readSerial(port,newSession("Live USB log"));
  } catch(err) {
    setStatus("Not connected",false,err.name==="NotFoundError"?"No serial port selected.":`Could not connect: ${err.message}. Close any other serial monitor using this port.`);
  } finally {byId("connect").disabled=false;}
});
async function sendCommand(command) {
  if(!serialPort?.writable||writeTask)return;
  byId("measure-color").disabled=true;byId("measure-lfa").disabled=true;
  const port=serialPort;
  writeTask=(async()=>{
    const writer=port.writable.getWriter();
    try {await writer.write(new TextEncoder().encode(command));byId("note").textContent=`Sent ${command}. Waiting for the firmware log…`;}
    finally {writer.releaseLock();}
  })();
  try {await writeTask;} catch(err){byId("note").textContent=`Could not send command: ${err.message}`;}
  finally {writeTask=null;if(serialPort){byId("measure-color").disabled=false;byId("measure-lfa").disabled=false;}}
}
byId("measure-color").addEventListener("click",()=>sendCommand("c"));
byId("measure-lfa").addEventListener("click",()=>sendCommand("l"));
if(!navigator.serial){byId("connect").disabled=true;byId("note").textContent="For live USB logs, open this localhost page in Chrome or Edge. Pasted/file logs work here.";}
Object.assign(window,{ingestLog,csvText,updateFreshness});
