#pragma once
#include <Arduino.h>

static const char HTML[] PROGMEM = R"HTMLEOF(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESPiBright</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700&family=DM+Sans:wght@400;500;600;700&display=swap');
:root{
  --bg:#080b0f;--panel:#0f1318;--panel2:#141920;
  --border:#1c2330;--border2:#253040;
  --accent:#00d4ff;--orange:#ff7340;--green:#00e08a;--purple:#b060ff;--yellow:#ffd060;
  --text:#b8c8dc;--bright:#e8f0f8;--dim:#4a5870;
  --mono:'JetBrains Mono',monospace;--sans:'DM Sans',sans-serif;
  --r:8px;--rs:5px;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:var(--sans);min-height:100vh;padding-bottom:80px}
::-webkit-scrollbar{width:5px;height:5px}
::-webkit-scrollbar-track{background:var(--bg)}
::-webkit-scrollbar-thumb{background:var(--border2);border-radius:3px}

/* Header */
header{background:var(--panel);border-bottom:1px solid var(--border);padding:13px 22px;
  display:flex;align-items:center;gap:14px;position:sticky;top:0;z-index:200}
.logo{width:30px;height:30px;border:2px solid var(--accent);border-radius:5px;
  display:flex;align-items:center;justify-content:center;
  font-family:var(--mono);font-size:13px;color:var(--accent);flex-shrink:0}
header h1{font-size:.9rem;font-weight:700;letter-spacing:.12em;text-transform:uppercase;color:#fff;flex:1}
.hdr-r{display:flex;align-items:center;gap:10px}
.pill{font-family:var(--mono);font-size:.62rem;padding:3px 9px;border-radius:20px;
  background:#0a1f14;color:var(--green);border:1px solid #00e08a33;white-space:nowrap}
.time-tog{display:flex;align-items:center;gap:7px;font-size:.72rem;color:var(--dim)}
.sw{position:relative;width:34px;height:19px;cursor:pointer}
.sw input{opacity:0;width:0;height:0}
.sw-tr{position:absolute;inset:0;background:var(--border2);border-radius:20px;transition:.18s}
.sw-tr:before{content:'';position:absolute;width:13px;height:13px;left:3px;bottom:3px;
  background:#fff;border-radius:50%;transition:.18s}
.sw input:checked+.sw-tr{background:var(--accent)}
.sw input:checked+.sw-tr:before{transform:translateX(15px)}

/* Layout */
main{max-width:940px;margin:0 auto;padding:22px 18px;display:flex;flex-direction:column;gap:24px}

/* Last TX */
.last-bar{background:var(--panel);border:1px solid var(--border);border-radius:var(--r);
  padding:11px 15px;display:flex;align-items:center;gap:10px;font-family:var(--mono);font-size:.7rem}
.lk{color:var(--dim);flex-shrink:0;font-size:.58rem;letter-spacing:.12em;text-transform:uppercase}
#last-label{color:var(--bright);font-weight:700}
#last-hex{margin-left:auto;color:var(--accent);letter-spacing:.05em;font-size:.66rem}
#last-ago{color:var(--dim);font-size:.58rem;flex-shrink:0}

/* Card */
.card{background:var(--panel);border:1px solid var(--border);border-radius:var(--r);overflow:hidden}
.card-hdr{padding:11px 17px;border-bottom:1px solid var(--border);
  display:flex;align-items:center;justify-content:space-between}
.card-title{font-size:.62rem;font-weight:700;letter-spacing:.18em;text-transform:uppercase;color:var(--dim)}
.card-body{padding:17px}

/* Channels */
.channels{display:flex;flex-direction:column;gap:11px}
.channel{background:var(--panel2);border:1px solid var(--border);border-radius:var(--rs);
  padding:12px 15px;display:grid;grid-template-columns:auto 1fr auto;
  align-items:center;gap:12px;transition:border-color .15s,opacity .2s}
.channel.off{opacity:.45}
.ch-l{display:flex;align-items:center;gap:9px}
.dot{width:9px;height:9px;border-radius:50%;flex-shrink:0}
.dot.w{background:#c8e0ff}
.dot.b{background:#4488ff}
.dot.r{background:conic-gradient(#ff4060,#00e08a,#4488ff,#ff4060)}
.ch-name{font-weight:600;font-size:.83rem;color:var(--bright);white-space:nowrap;width:52px}
.ch-m{display:flex;align-items:center;gap:9px;min-width:0;flex:1}
input[type=range]{flex:1;-webkit-appearance:none;height:4px;border-radius:2px;
  background:var(--border2);outline:none;cursor:pointer;min-width:60px}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:15px;height:15px;
  border-radius:50%;background:var(--accent);cursor:pointer;transition:transform .1s}
input[type=range]::-webkit-slider-thumb:hover{transform:scale(1.2)}
.sl-b::-webkit-slider-thumb{background:#4488ff!important}
.sl-r::-webkit-slider-thumb{background:var(--purple)!important}
.num{width:50px;background:var(--bg);border:1px solid var(--border);color:var(--bright);
  font-family:var(--mono);font-size:.82rem;text-align:center;border-radius:4px;padding:5px 4px;outline:none;
  -moz-appearance:textfield}
.num::-webkit-inner-spin-button{-webkit-appearance:none}
.num:focus{border-color:var(--accent)}
.pct{color:var(--dim);font-size:.68rem;flex-shrink:0}
.ch-r{display:flex;align-items:center}
.tog-btn{display:flex;background:var(--bg);border:1px solid var(--border);border-radius:20px;overflow:hidden}
.tog-btn button{border:none;padding:5px 11px;font-size:.68rem;font-weight:700;font-family:var(--sans);
  cursor:pointer;background:transparent;color:var(--dim);transition:background .12s,color .12s;letter-spacing:.05em}
.tog-btn button.on.a{background:var(--green);color:#000}
.tog-btn button.off.a{background:var(--border2);color:var(--bright)}

/* Buttons */
.btn{font-family:var(--sans);font-weight:700;font-size:.8rem;letter-spacing:.06em;text-transform:uppercase;
  border:none;border-radius:var(--rs);padding:10px 20px;cursor:pointer;transition:opacity .15s,transform .1s}
.btn:active{transform:scale(.97)}
.btn:disabled{opacity:.3;cursor:default}
.btn-p{background:var(--accent);color:#000}.btn-p:hover{opacity:.85}
.btn-s{background:var(--panel2);color:var(--text);border:1px solid var(--border2)}
.btn-s:hover{border-color:var(--accent);color:var(--bright)}
.btn-o{background:var(--orange);color:#000}.btn-o:hover{opacity:.85}
.btn-sm{padding:7px 13px;font-size:.7rem}
.send-row{display:flex;gap:9px;align-items:center;margin-top:14px;flex-wrap:wrap}

/* Time */
.time-grid{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.tf{display:flex;flex-direction:column;gap:5px}
.tf label{font-size:.58rem;letter-spacing:.14em;text-transform:uppercase;color:var(--dim)}
.tf input{width:68px;background:var(--bg);border:1px solid var(--border);color:var(--accent);
  font-family:var(--mono);font-size:1.35rem;text-align:center;border-radius:var(--rs);padding:7px;outline:none;
  -moz-appearance:textfield}
.tf input::-webkit-inner-spin-button{-webkit-appearance:none}
.tf input:focus{border-color:var(--accent)}
.tsep{font-family:var(--mono);font-size:1.35rem;color:var(--dim);padding-top:17px}
#time-display{font-family:var(--mono);font-size:1.6rem;color:var(--bright);letter-spacing:.08em}

/* Schedule */
/* Schedule */
.sched-grid{display:flex;flex-direction:column;gap:18px}
.sched-group{display:flex;flex-direction:column;gap:8px}
.sched-group-title{font-size:.58rem;letter-spacing:.18em;text-transform:uppercase;font-weight:700;margin-bottom:2px}
.sched-group-title.off{color:var(--orange)}
.sched-group-title.on {color:var(--green)}
.sched-group-title.state{color:var(--purple)}
.sched-row{
  display:flex;align-items:center;gap:10px;flex-wrap:wrap;
  background:var(--panel2);border:1px solid var(--border);border-radius:var(--rs);
  padding:9px 12px;transition:border-color .12s;
}
.sched-row.enabled{border-left:3px solid var(--border2)}
.sched-row.enabled.off-row{border-left-color:var(--orange)}
.sched-row.enabled.on-row {border-left-color:var(--green)}
.sched-type{font-family:var(--mono);font-size:.65rem;color:var(--dim);width:54px;flex-shrink:0}
.sched-time{display:flex;align-items:center;gap:5px}
.sched-time input{
  width:48px;background:var(--bg);border:1px solid var(--border);
  color:var(--bright);font-family:var(--mono);font-size:.9rem;text-align:center;
  border-radius:4px;padding:5px;outline:none;-moz-appearance:textfield;
}
.sched-time input::-webkit-inner-spin-button{-webkit-appearance:none}
.sched-time input:focus{border-color:var(--accent)}
.sched-time .sep{font-family:var(--mono);color:var(--dim);font-size:.9rem}
.sched-inactive{opacity:.4}
.sched-foot{display:flex;gap:10px;margin-top:14px;align-items:center;flex-wrap:wrap}
.sched-note{font-size:.7rem;color:var(--dim);flex:1}
.badge{font-family:var(--mono);font-size:.66rem;padding:2px 7px;border-radius:3px;font-weight:700}
.b-on{background:#0a2018;color:var(--green)}
.b-off{background:#1a1008;color:var(--orange)}
.state-preview{
  font-family:var(--mono);font-size:.65rem;color:var(--dim);
  background:var(--bg);border:1px solid var(--border);border-radius:4px;
  padding:3px 8px;white-space:nowrap;
}


/* Known packets */
.pkt-grid-wrap{display:flex;flex-wrap:wrap;gap:7px}
.pkt-btn{background:var(--panel2);border:1px solid var(--border);color:var(--text);
  font-family:var(--sans);font-size:.76rem;font-weight:600;padding:8px 13px;border-radius:var(--rs);
  cursor:pointer;transition:border-color .12s,background .12s,transform .1s;position:relative;overflow:hidden}
.pkt-btn:hover{border-color:var(--accent);background:#0a1a22}
.pkt-btn:active{transform:scale(.96)}
.pkt-btn.firing{border-color:var(--accent);animation:pulse .4s ease}
.pkt-btn .tbadge{position:absolute;top:3px;right:4px;font-size:.46rem;color:var(--orange);font-family:var(--mono)}
.gPower   .pkt-btn{border-left:3px solid var(--orange)}
.gDim     .pkt-btn{border-left:3px solid var(--yellow)}
.gChannel .pkt-btn{border-left:3px solid var(--accent)}
.gSchedule .pkt-btn{border-left:3px solid var(--purple)}
.gTime    .pkt-btn{border-left:3px solid var(--green)}
@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(0,212,255,.5)}70%{box-shadow:0 0 0 8px rgba(0,212,255,0)}100%{box-shadow:0 0 0 0 rgba(0,212,255,0)}}

/* Crafter */
.crafter-bytes{display:flex;gap:7px;flex-wrap:wrap;margin-bottom:13px}
.bc{display:flex;flex-direction:column;align-items:center;gap:4px}
.bc label{font-family:var(--mono);font-size:.56rem;color:var(--dim);text-transform:uppercase}
.bc input{width:44px;height:36px;background:var(--bg);border:1px solid var(--border);
  color:var(--accent);font-family:var(--mono);font-size:.95rem;text-align:center;border-radius:4px;outline:none;transition:border-color .13s}
.bc input:focus{border-color:var(--accent)}
.bc input.fixed{color:var(--dim);cursor:default}
.bc input.crc{color:var(--green);cursor:default}
.bc input.bad{border-color:#ff4040}
.craft-opts{display:flex;gap:12px;align-items:center;flex-wrap:wrap}
.craft-opts label{display:flex;align-items:center;gap:6px;font-size:.76rem;color:var(--dim);cursor:pointer}
.craft-opts input[type=checkbox]{accent-color:var(--orange)}

/* Terminal log */
.terminal{background:#020507;border:1px solid var(--border);border-radius:var(--r);overflow:hidden}
.term-hdr{padding:10px 15px;border-bottom:1px solid var(--border);display:flex;align-items:center;gap:10px}
.term-dots{display:flex;gap:5px}
.term-dot{width:9px;height:9px;border-radius:50%}
.term-body{height:280px;overflow-y:auto;padding:10px 14px;font-family:var(--mono);font-size:.72rem;
  line-height:1.6;scroll-behavior:smooth}
.term-body::-webkit-scrollbar{width:4px}
.term-body::-webkit-scrollbar-track{background:#020507}
.term-body::-webkit-scrollbar-thumb{background:#1c2330;border-radius:2px}
.log-entry{margin-bottom:10px;border-left:2px solid var(--border2);padding-left:10px}
.log-entry:last-child{border-left-color:var(--accent)}
.log-ts{color:var(--dim);font-size:.62rem}
.log-label{color:var(--bright);font-weight:700;margin-left:8px}
.log-pkt{display:flex;align-items:baseline;gap:10px;margin-top:3px}
.log-note{color:var(--dim);font-size:.62rem;width:52px;flex-shrink:0;text-align:right}
.log-hex{color:var(--accent);letter-spacing:.06em;font-size:.7rem}
.log-hex span{color:var(--dim)}
.log-empty{color:var(--dim);font-style:italic;padding:20px 0;text-align:center}
.term-footer{padding:8px 14px;border-top:1px solid var(--border);display:flex;align-items:center;gap:10px}
.term-count{font-family:var(--mono);font-size:.62rem;color:var(--dim);flex:1}
#auto-scroll-lbl{display:flex;align-items:center;gap:6px;font-size:.72rem;color:var(--dim);cursor:pointer}

/* API table entry for log */
.api-tbl{width:100%;border-collapse:collapse;font-size:.78rem}
.api-tbl th{text-align:left;font-size:.56rem;letter-spacing:.14em;text-transform:uppercase;
  color:var(--dim);border-bottom:1px solid var(--border);padding:6px 7px}
.api-tbl td{padding:8px 7px;border-bottom:1px solid var(--border);vertical-align:top}
.api-tbl tr:last-child td{border-bottom:none}
.m{font-family:var(--mono);font-size:.66rem;font-weight:700;padding:2px 6px;border-radius:3px;white-space:nowrap}
.mG{background:#0a2018;color:var(--green)}
.mP{background:#1a1a00;color:var(--yellow)}
.ep{font-family:var(--mono);color:var(--accent);font-size:.76rem}
.ad{color:var(--dim);font-size:.73rem}
code{font-family:var(--mono);font-size:.7rem;background:var(--bg);padding:1px 5px;border-radius:3px;color:var(--text)}

/* Collapsible */
details summary{list-style:none;cursor:pointer;user-select:none}
details summary::-webkit-details-marker{display:none}
.sum-row{display:flex;align-items:center;justify-content:space-between}
.chev{font-size:.65rem;color:var(--dim);transition:transform .18s}
details[open] .chev{transform:rotate(180deg)}

/* Color pills */
.color-pills{display:flex;flex-wrap:wrap;gap:6px}
.color-pill{
  border:1px solid var(--border2);border-radius:20px;
  padding:4px 12px;font-size:.72rem;font-weight:600;font-family:var(--sans);
  cursor:pointer;transition:border-color .12s,transform .1s,opacity .12s;
  background:var(--panel2);color:var(--text);letter-spacing:.04em;
}
.color-pill:active{transform:scale(.95)}
.color-pill.sel{border-color:transparent;color:#000;font-weight:700}
.cp-blue   {--pc:#4488ff}.cp-green  {--pc:#00e08a}.cp-white  {--pc:#d0e8ff}
.cp-red    {--pc:#ff4060}.cp-orange {--pc:#ff7340}.cp-purple {--pc:#b060ff}
.cp-pink   {--pc:#ff70c0}.cp-yellow {--pc:#ffd060}.cp-rainbow{--pc:transparent}
.color-pill:not(.sel):hover{border-color:var(--pc,var(--accent));color:var(--pc,var(--accent))}
.cp-blue.sel   {background:#4488ff}.cp-green.sel  {background:#00e08a}
.cp-white.sel  {background:#d0e8ff}.cp-red.sel    {background:#ff4060}
.cp-orange.sel {background:#ff7340}.cp-purple.sel {background:#b060ff}
.cp-pink.sel   {background:#ff70c0}.cp-yellow.sel {background:#ffd060}
.cp-rainbow.sel{background:linear-gradient(90deg,#ff4060,#ff7340,#ffd060,#00e08a,#4488ff,#b060ff,#ff70c0);color:#fff!important}

/* Group label */
.grp-lbl{font-size:.58rem;letter-spacing:.17em;text-transform:uppercase;color:var(--dim);margin-bottom:7px}

/* Toast */
#toast{position:fixed;bottom:18px;left:50%;transform:translateX(-50%) translateY(60px);
  background:var(--panel);border:1px solid var(--accent);color:var(--accent);
  font-family:var(--mono);font-size:.76rem;padding:9px 18px;border-radius:6px;
  transition:transform .22s cubic-bezier(.34,1.56,.64,1),opacity .18s;opacity:0;z-index:999;white-space:nowrap}
#toast.show{transform:translateX(-50%) translateY(0);opacity:1}
#toast.err{border-color:var(--orange);color:var(--orange)}
</style>
</head>
<body>
<header>
  <div class="logo">&#9685;</div>
  <h1>ESPiBright</h1>
  <div class="hdr-r">
    <div class="time-tog">
      <span>+TIME</span>
      <label class="sw">
        <input type="checkbox" id="global-time" checked onchange="setGlobalTime(this.checked)">
        <span class="sw-tr"></span>
      </label>
    </div>
    <div class="pill">&#9679; CONNECTED</div>
  </div>
</header>

<main>

<!-- Last TX -->
<div class="last-bar">
  <span class="lk">Last TX</span>
  <span id="last-label">—</span>
  <span id="last-hex">—</span>
  <span id="last-ago"></span>
</div>

<!-- Channel Controls -->
<div class="card">
  <div class="card-hdr">
    <span class="card-title">Channel Controls</span>
  </div>
  <div class="card-body">
    <div class="channels">

      <div class="channel" id="ch-white">
        <div class="ch-l"><div class="dot w"></div><span class="ch-name">White</span></div>
        <div class="ch-m">
          <input type="range" min="1" max="10" value="10" id="sl-white" oninput="syncPct('white',this.value)">
          <input type="number" min="10" max="100" step="10" value="100" id="pct-white" class="num"
            onchange="syncSlider('white',this.value)">
          <span class="pct">%</span>
        </div>
        <div class="ch-r">
          <div class="tog-btn">
            <button class="on a" onclick="setCh('white',true)">ON</button>
            <button class="off"  onclick="setCh('white',false)">OFF</button>
          </div>
        </div>
      </div>

      <div class="channel" id="ch-blue">
        <div class="ch-l"><div class="dot b"></div><span class="ch-name">Blue</span></div>
        <div class="ch-m">
          <input type="range" min="1" max="10" value="10" id="sl-blue" oninput="syncPct('blue',this.value)" class="sl-b">
          <input type="number" min="10" max="100" step="10" value="100" id="pct-blue" class="num"
            onchange="syncSlider('blue',this.value)">
          <span class="pct">%</span>
        </div>
        <div class="ch-r">
          <div class="tog-btn">
            <button class="on a" onclick="setCh('blue',true)">ON</button>
            <button class="off"  onclick="setCh('blue',false)">OFF</button>
          </div>
        </div>
      </div>

      <div class="channel" id="ch-rgb" style="display:flex;flex-direction:column;gap:10px">
        <div style="display:flex;align-items:center;gap:12px">
          <div class="ch-l"><div class="dot r"></div><span class="ch-name">RGB</span></div>
          <div style="flex:1;display:flex;align-items:center;gap:9px">
            <span style="font-size:.72rem;color:var(--dim)">Speed</span>
            <select id="rgb-cycle" class="num" style="font-size:.72rem;padding:2px 4px;background:var(--panel2);color:var(--bright);border:1px solid var(--border);border-radius:4px;cursor:pointer" onchange="updateSchedStatePreviews()">
              <option value="1">Static</option>
              <option value="3">3s</option>
              <option value="5">4s</option>
              <option value="9">5s</option>
              <option value="17">6s?</option>
            </select>
          </div>
          <div class="ch-r">
            <div class="tog-btn">
              <button class="on a" onclick="setCh('rgb',true)">ON</button>
              <button class="off"  onclick="setCh('rgb',false)">OFF</button>
            </div>
          </div>
        </div>
        <div class="color-pills" id="color-pills"></div>
      </div>

    </div>
    <div class="send-row">
      <button class="btn btn-p" onclick="sendChannels()">Send ×5</button>
      <button class="btn btn-s btn-sm" onclick="allOff()">All Off</button>
    </div>
  </div>
</div>

<!-- Time Setter -->
<div class="card">
  <div class="card-hdr">
    <span class="card-title">Device Time</span>
    <span id="time-display">--:--:--</span>
  </div>
  <div class="card-body">
    <div class="time-grid">
      <div class="tf"><label>Hour</label><input type="number" id="t-hh" min="0" max="23" value="19" oninput="updTDisp()"></div>
      <span class="tsep">:</span>
      <div class="tf"><label>Min</label><input type="number" id="t-mm" min="0" max="59" value="20" oninput="updTDisp()"></div>
      <span class="tsep">:</span>
      <div class="tf"><label>Sec</label><input type="number" id="t-ss" min="0" max="59" value="29" oninput="updTDisp()"></div>
      <div style="display:flex;flex-direction:column;gap:7px;padding-top:18px">
        <button class="btn btn-p btn-sm" onclick="setTime()">Set Time</button>
        <button class="btn btn-s btn-sm" onclick="useBrowserTime()">Browser Time</button>
      </div>
    </div>
    <div class="send-row">
      <button class="btn btn-s btn-sm" onclick="sendTimeOnly()">Send Time Packets Now</button>
    </div>
  </div>
</div>

<!-- Schedule Builder -->
<div class="card">
  <div class="card-hdr">
    <span class="card-title">Schedule Builder</span>
    <span style="font-size:.65rem;color:var(--dim)">sequence ×3 per send</span>
  </div>
  <div class="card-body">
    <div class="sched-grid">

      <!-- Header row -->
      <div style="display:grid;grid-template-columns:28px 26px 1fr 1fr;gap:8px;padding:0 4px">
        <div></div><div></div>
        <div style="font-size:.6rem;letter-spacing:.14em;text-transform:uppercase;color:var(--green);text-align:center;font-weight:700">ON</div>
        <div style="font-size:.6rem;letter-spacing:.14em;text-transform:uppercase;color:var(--orange);text-align:center;font-weight:700">OFF</div>
      </div>

      <!-- White row -->
      <div style="display:grid;grid-template-columns:28px 26px 1fr 1fr;gap:8px;align-items:center">
        <div style="width:9px;height:9px;border-radius:50%;background:#c8e0ff;margin:auto"></div>
        <span style="font-size:.75rem;font-weight:600;color:var(--bright)">W</span>
        <div class="sched-row on-row" id="sw-on">
          <input type="checkbox" id="sw-on-en" checked onchange="schedRowToggle('sw-on')" style="accent-color:var(--green);width:14px;height:14px;cursor:pointer;flex-shrink:0">
          <span class="sched-type" style="color:var(--dim)">04</span>
          <div class="sched-time"><input type="number" id="sw-on-hh" min="0" max="23" value="11"><span class="sep">:</span><input type="number" id="sw-on-mm" min="0" max="59" value="0"></div>
        </div>
        <div class="sched-row off-row" id="sw-off">
          <input type="checkbox" id="sw-off-en" checked onchange="schedRowToggle('sw-off')" style="accent-color:var(--orange);width:14px;height:14px;cursor:pointer;flex-shrink:0">
          <span class="sched-type" style="color:var(--dim)">03</span>
          <div class="sched-time"><input type="number" id="sw-off-hh" min="0" max="23" value="23"><span class="sep">:</span><input type="number" id="sw-off-mm" min="0" max="59" value="0"></div>
        </div>
      </div>

      <!-- Blue row -->
      <div style="display:grid;grid-template-columns:28px 26px 1fr 1fr;gap:8px;align-items:center">
        <div style="width:9px;height:9px;border-radius:50%;background:#4488ff;margin:auto"></div>
        <span style="font-size:.75rem;font-weight:600;color:var(--bright)">B</span>
        <div class="sched-row on-row" id="sb-on">
          <input type="checkbox" id="sb-on-en" checked onchange="schedRowToggle('sb-on')" style="accent-color:var(--green);width:14px;height:14px;cursor:pointer;flex-shrink:0">
          <span class="sched-type" style="color:var(--dim)">05</span>
          <div class="sched-time"><input type="number" id="sb-on-hh" min="0" max="23" value="11"><span class="sep">:</span><input type="number" id="sb-on-mm" min="0" max="59" value="0"></div>
        </div>
        <div class="sched-row off-row" id="sb-off">
          <input type="checkbox" id="sb-off-en" checked onchange="schedRowToggle('sb-off')" style="accent-color:var(--orange);width:14px;height:14px;cursor:pointer;flex-shrink:0">
          <span class="sched-type" style="color:var(--dim)">02</span>
          <div class="sched-time"><input type="number" id="sb-off-hh" min="0" max="23" value="23"><span class="sep">:</span><input type="number" id="sb-off-mm" min="0" max="59" value="30"></div>
        </div>
      </div>

      <!-- RGB row -->
      <div style="display:grid;grid-template-columns:28px 26px 1fr 1fr;gap:8px;align-items:start">
        <div style="width:9px;height:9px;border-radius:50%;background:conic-gradient(#ff4060,#ffd060,#00e08a,#4488ff,#ff4060);margin:auto;margin-top:12px"></div>
        <span style="font-size:.75rem;font-weight:600;color:var(--bright);margin-top:10px">RGB</span>
        <div style="display:flex;flex-direction:column;gap:5px">
          <div class="sched-row on-row" id="sr-on">
            <input type="checkbox" id="sr-on-en" checked onchange="schedRowToggle('sr-on')" style="accent-color:var(--green);width:14px;height:14px;cursor:pointer;flex-shrink:0">
            <span class="sched-type" style="color:var(--dim)">08</span>
            <div class="sched-time"><input type="number" id="sr-on-hh" min="0" max="23" value="11"><span class="sep">:</span><input type="number" id="sr-on-mm" min="0" max="59" value="0"></div>
          </div>
          <div class="color-pills" id="sr-on-pills" style="padding:0 4px"></div>
        </div>
        <div style="display:flex;flex-direction:column;gap:5px">
          <div class="sched-row off-row" id="sr-off">
            <input type="checkbox" id="sr-off-en" checked onchange="schedRowToggle('sr-off')" style="accent-color:var(--orange);width:14px;height:14px;cursor:pointer;flex-shrink:0">
            <span class="sched-type" style="color:var(--dim)">09</span>
            <div class="sched-time"><input type="number" id="sr-off-hh" min="0" max="23" value="23"><span class="sep">:</span><input type="number" id="sr-off-mm" min="0" max="59" value="0"></div>
          </div>
          <div class="color-pills" id="sr-off-pills" style="padding:0 4px"></div>
        </div>
      </div>

      <!-- Type 07 state preview -->
      <div style="display:flex;align-items:center;gap:10px;padding:8px 12px;background:var(--panel2);border:1px solid var(--border);border-left:3px solid var(--purple);border-radius:var(--rs)">
        <span class="sched-type" style="color:var(--purple)">07</span>
        <span class="state-preview" id="sched-state-preview">B3 = 0x86</span>
        <span style="font-size:.68rem;color:var(--dim)">live broadcast — auto from current RGB channel, not stored</span>
      </div>

    </div><!-- /sched-grid -->
    <div class="sched-foot">
      <span class="sched-note">RGB ON/OFF store their own color (frozen at save time), independent of live channel. Type 07 = live state. Type 02 = Blue OFF (last in sequence). Sequence x3.</span>
      <button class="btn btn-o btn-sm" onclick="sendSchedule()">Send Schedule</button>
    </div>
  </div>
</div>

<!-- TX Terminal -->
<div class="terminal">
  <div class="term-hdr">
    <div class="term-dots">
      <div class="term-dot" style="background:#ff5f57"></div>
      <div class="term-dot" style="background:#febc2e"></div>
      <div class="term-dot" style="background:#28c840"></div>
    </div>
    <span style="font-family:var(--mono);font-size:.68rem;color:var(--dim);flex:1">TX PACKET LOG</span>
    <button class="btn btn-s btn-sm" onclick="clearLog()" style="padding:4px 10px;font-size:.62rem">Clear</button>
  </div>
  <div class="term-body" id="term-body">
    <div class="log-empty">No transmissions yet</div>
  </div>
  <div class="term-footer">
    <span class="term-count" id="term-count">0 entries</span>
    <label id="auto-scroll-lbl">
      <input type="checkbox" id="auto-scroll" checked style="accent-color:var(--accent)">
      auto-scroll
    </label>
  </div>
</div>

<!-- Known Packets -->
<div class="card">
  <details>
    <summary>
      <div class="card-hdr sum-row" style="border-bottom:none">
        <span class="card-title">Known Packets</span>
        <span class="chev">&#9660;</span>
      </div>
    </summary>
    <div class="card-body" id="pkt-container"></div>
  </details>
</div>

<!-- Packet Crafter -->
<div class="card">
  <details>
    <summary>
      <div class="card-hdr sum-row" style="border-bottom:none">
        <span class="card-title">Packet Crafter</span>
        <span class="chev">&#9660;</span>
      </div>
    </summary>
    <div class="card-body">
      <div class="crafter-bytes" id="crafter-bytes"></div>
      <div class="craft-opts">
        <button class="btn btn-p" onclick="sendCraft()">Send ×5</button>
        <label><input type="checkbox" id="craft-time" checked> append time packets</label>
      </div>
    </div>
  </details>
</div>

<!-- REST API -->
<div class="card">
  <details>
    <summary>
      <div class="card-hdr sum-row" style="border-bottom:none">
        <span class="card-title">REST API</span>
        <span class="chev">&#9660;</span>
      </div>
    </summary>
    <div class="card-body">
      <table class="api-tbl">
        <thead><tr><th>Method</th><th>Endpoint</th><th>Description</th></tr></thead>
        <tbody>
          <tr><td><span class="m mG">GET</span></td><td class="ep">/api/log?since=N</td><td class="ad">Returns up to 40 TX log entries newer than sequence N. Each entry includes label + all packets sent (cmd, time, schedule).</td></tr>
          <tr><td><span class="m mG">GET</span></td><td class="ep">/api/packets</td><td class="ad">All known packets as JSON.</td></tr>
          <tr><td><span class="m mG">GET</span></td><td class="ep">/api/status</td><td class="ad">Last TX, current time, global toggle.</td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/send/index</td><td class="ad">Known packet by index. <code>{"index":0}</code></td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/send/raw</td><td class="ad">7-byte payload (CRC auto). <code>{"payload":"d0238a8a8601a6","send_time":true}</code></td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/send/channels</td><td class="ad">Channel state. <code>{"white_on":true,"white_level":10,"blue_on":true,"blue_level":10,"rgb_on":true,"rgb_color":8,"rgb_cycle":1,"rgb_level":10}</code><br>Levels 1–10 (10%–100%). rgb_color: 1=blue 2=green 3=white 4=red 5=orange 6=purple 7=pink 8=yellow 9=rainbow. rgb_cycle (rainbow only): 1=static, 2=3s, 4=4s, 8=5s.</td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/time/set</td><td class="ad">Set time. <code>{"hh":19,"mm":20,"ss":29}</code></td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/time/send</td><td class="ad">Transmit time packets immediately.</td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/schedule/set</td><td class="ad">Set schedule slots.<br><code>{"off":[{"active":true,"hh":23,"mm":0},{"active":true,"hh":11,"mm":0},{"active":false,"hh":0,"mm":0}],"on":[{"active":true,"hh":11,"mm":0},{"active":true,"hh":23,"mm":0}]}</code><br>State byte for ON slots and type 07 auto-computed from current channel state.</td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/schedule/send</td><td class="ad">Transmit all active schedule slots.</td></tr>
          <tr><td><span class="m mP">POST</span></td><td class="ep">/api/settings/time_global</td><td class="ad">Global time toggle. <code>{"enabled":true}</code></td></tr>
        </tbody>
      </table>
    </div>
  </details>
</div>

</main>
<div id="toast"></div>

<script>
const PACKETS = window.__PACKETS__ || [];

// ── Channel state ──
const CH = {
  white:{on:true,level:10},
  blue: {on:true,level:10},
  rgb:  {on:true,color:8,cycle:1}
};

function syncPct(ch,v){
  document.getElementById('pct-'+ch).value=v*10;
  CH[ch].level=parseInt(v);
}
function syncSlider(ch,v){
  const c=Math.min(100,Math.max(10,Math.round(v/10)*10));
  document.getElementById('pct-'+ch).value=c;
  document.getElementById('sl-'+ch).value=c/10;
  CH[ch].level=c/10;
}
function setCh(ch,on){
  CH[ch].on=on;
  const el=document.getElementById('ch-'+ch);
  el.classList.toggle('off',!on);
  el.querySelectorAll('.tog-btn button').forEach(b=>{
    b.classList.toggle('a',b.classList.contains(on?'on':'off'));
  });
  if(ch==='rgb') updateSchedStatePreviews();
}

// ── RGB color pills ──
const RGB_COLORS = [
  {v:1,label:'Blue',   cls:'cp-blue'},
  {v:2,label:'Green',  cls:'cp-green'},
  {v:3,label:'White',  cls:'cp-white'},
  {v:4,label:'Red',    cls:'cp-red'},
  {v:5,label:'Orange', cls:'cp-orange'},
  {v:6,label:'Purple', cls:'cp-purple'},
  {v:7,label:'Pink',   cls:'cp-pink'},
  {v:8,label:'Yellow', cls:'cp-yellow'},
  {v:9,label:'Rainbow',cls:'cp-rainbow'},
];
const pillContainer = document.getElementById('color-pills');
RGB_COLORS.forEach(c => {
  const btn = document.createElement('button');
  btn.className = 'color-pill '+c.cls;
  btn.textContent = c.label;
  btn.dataset.val = c.v;
  if (c.v === CH.rgb.color) btn.classList.add('sel');
  btn.onclick = () => {
    CH.rgb.color = c.v;
    pillContainer.querySelectorAll('.color-pill').forEach(p => p.classList.remove('sel'));
    btn.classList.add('sel');
    updateSchedStatePreviews();
  };
  pillContainer.appendChild(btn);
});

function updateRgbDurationRow(){
  const isRainbow = CH.rgb.color === 9;
  const row = document.getElementById('rgb-cycle-row');
  if(row) row.style.display = isRainbow ? 'flex' : 'none';
  if(!isRainbow) CH.rgb.cycle = 1;
}

async function sendChannels(){
  if(CH.rgb.color===9){
    CH.rgb.cycle=parseInt(document.getElementById('rgb-cycle').value)||2;
  } else {
    CH.rgb.cycle=1;
  }
  const r=await api('/api/send/channels','POST',{
    white_on:CH.white.on,white_level:CH.white.level,
    blue_on:CH.blue.on,blue_level:CH.blue.level,
    rgb_on:CH.rgb.on,rgb_color:CH.rgb.color,rgb_cycle:CH.rgb.cycle,rgb_level:CH.rgb.level
  });
  r.ok?toast('✓ channels sent'):toast('✗ '+(r.error||'error'),true);
}
async function allOff(){
  ['white','blue','rgb'].forEach(c=>setCh(c,false));
  await sendChannels();
}

// ── Time ──
function pad(n){return String(n).padStart(2,'0')}
function updTDisp(){
  document.getElementById('time-display').textContent=
    pad(document.getElementById('t-hh').value)+':'+
    pad(document.getElementById('t-mm').value)+':'+
    pad(document.getElementById('t-ss').value);
}
function useBrowserTime(){
  const n=new Date();
  document.getElementById('t-hh').value=n.getHours();
  document.getElementById('t-mm').value=n.getMinutes();
  document.getElementById('t-ss').value=n.getSeconds();
  updTDisp();
}
async function setTime(){
  const r=await api('/api/time/set','POST',{
    hh:parseInt(document.getElementById('t-hh').value),
    mm:parseInt(document.getElementById('t-mm').value),
    ss:parseInt(document.getElementById('t-ss').value)
  });
  r.ok?toast('✓ time set'):toast('✗ '+(r.error||'error'),true);
}
async function sendTimeOnly(){
  const r=await api('/api/time/send','POST',{});
  r.ok?toast('✓ time packets sent'):toast('✗ '+(r.error||'error'),true);
}

// ── Schedule ──
// ── Schedule ──
function schedRowToggle(id) {
  const en = document.getElementById(id+'-en').checked;
  document.getElementById(id).classList.toggle('sched-inactive', !en);
}

// Per-slot stored RGB state (independent of live channel)
const schedRgbState = { 'sr-on': 0x86, 'sr-off': 0x86 };

function buildSchedPills(containerId) {
  const container = document.getElementById(containerId);
  if (!container) return;
  RGB_COLORS.forEach(c => {
    const btn = document.createElement('button');
    btn.className = 'color-pill '+c.cls;
    btn.style.cssText = 'padding:2px 8px;font-size:.62rem';
    btn.textContent = c.label;
    btn.dataset.val = c.v;
    const slotId = containerId.replace('-pills','');
    const stateVal = schedRgbState[slotId] || 0x86;
    const curColor = stateVal & 0x0f;
    if (c.v === curColor) btn.classList.add('sel');
    btn.onclick = () => {
      const on = (schedRgbState[slotId] & 0x80);
      schedRgbState[slotId] = on | (c.v & 0x0f);
      container.querySelectorAll('.color-pill').forEach(p => p.classList.remove('sel'));
      btn.classList.add('sel');
    };
    container.appendChild(btn);
  });
}
buildSchedPills('sr-on-pills');
buildSchedPills('sr-off-pills');

function rgbStateByte() {
  return (CH.rgb.on ? 0x80:0x00) | (CH.rgb.color & 0x0f);
}

function updateSchedStatePreviews() {
  const b = rgbStateByte();
  const hex = '0x'+b.toString(16).padStart(2,'0').toUpperCase();
  const colors={1:'blue',2:'green',3:'white',4:'red',5:'orange',6:'purple',7:'pink',8:'yellow',9:'rainbow'};
  const el = document.getElementById('sched-state-preview');
  if(el) el.textContent = 'B3 = '+hex+' (RGB '+(CH.rgb.on?'on, '+(colors[CH.rgb.color]||'?'):'off')+') — live';
}
updateSchedStatePreviews();

function g(id) { return document.getElementById(id); }
function slotVal(prefix) {
  return {
    active: g(prefix+'-en').checked,
    hh: parseInt(g(prefix+'-hh').value)||0,
    mm: parseInt(g(prefix+'-mm').value)||0,
  };
}

function readSched() {
  return {
    white_on:  slotVal('sw-on'),
    white_off: slotVal('sw-off'),
    blue_on:   slotVal('sb-on'),
    blue_off:  slotVal('sb-off'),
    rgb_on:    {...slotVal('sr-on'),  state: schedRgbState['sr-on']},
    rgb_off:   {...slotVal('sr-off'), state: schedRgbState['sr-off']},
  };
}

async function sendSchedule(){
  updateSchedStatePreviews();
  const r = await api('/api/schedule/set','POST', readSched());
  if(!r.ok){toast('✗ set failed',true);return;}
  const r2 = await api('/api/schedule/send','POST',{});
  r2.ok?toast('✓ schedule sent (×3)'):toast('✗ '+(r2.error||'error'),true);
}

// ── Known packets grid ──
const groups=['Power','Dim','Channel','Schedule','Time'];
const byGroup={};
PACKETS.forEach(p=>(byGroup[p.group]=byGroup[p.group]||[]).push(p));
const cont=document.getElementById('pkt-container');
groups.forEach(g=>{
  if(!byGroup[g])return;
  const wrap=document.createElement('div');
  wrap.style.marginBottom='15px';
  const lbl=document.createElement('div');lbl.className='grp-lbl';lbl.textContent=g;
  const grid=document.createElement('div');grid.className='pkt-grid-wrap g'+g;
  byGroup[g].forEach(p=>{
    const btn=document.createElement('button');btn.className='pkt-btn';
    btn.textContent=p.label;
    if(p.send_time){const s=document.createElement('span');s.className='tbadge';s.textContent='+T';btn.appendChild(s);}
    btn.onclick=()=>sendIndex(p.index,btn);
    grid.appendChild(btn);
  });
  wrap.appendChild(lbl);wrap.appendChild(grid);cont.appendChild(wrap);
});
async function sendIndex(idx,btn){
  btn&&btn.classList.add('firing');
  const r=await api('/api/send/index','POST',{index:idx});
  if(r.ok){toast('✓ '+r.label);updLastBar(r.label,r.hex);}
  else toast('✗ '+(r.error||'error'),true);
  setTimeout(()=>btn&&btn.classList.remove('firing'),500);
}

// ── Packet crafter ──
const BLABELS=['B1','B2','B3','B4','B5','B6','B7','CRC'];
const BDEFS=['d0','23','8a','8a','86','01','a6',''];
const bEl=document.getElementById('crafter-bytes');
const bInputs=[];
BLABELS.forEach((l,i)=>{
  const c=document.createElement('div');c.className='bc';
  const lb=document.createElement('label');lb.textContent=l;
  const inp=document.createElement('input');inp.maxLength=2;inp.value=BDEFS[i];
  if(i<2){inp.classList.add('fixed');inp.readOnly=true;}
  if(i===7){inp.classList.add('crc');inp.readOnly=true;inp.placeholder='auto';}
  inp.oninput=updCraft;
  c.appendChild(lb);c.appendChild(inp);bEl.appendChild(c);bInputs.push(inp);
});
function updCraft(){
  let ok=true;
  bInputs.slice(2,7).forEach(inp=>{
    const v=/^[0-9a-fA-F]{2}$/.test(inp.value.trim());
    inp.classList.toggle('bad',!v&&inp.value.length>0);
    if(!v)ok=false;
  });
  if(ok)bInputs[7].value='??';
}
updCraft();
async function sendCraft(){
  const payload=bInputs.slice(0,7).map(i=>i.value.trim()).join('');
  const r=await api('/api/send/raw','POST',{payload,send_time:document.getElementById('craft-time').checked});
  if(r.ok){toast('✓ sent');bInputs[7].value=r.hex?r.hex.slice(14,16):'??';updLastBar('CRAFT',r.hex);}
  else toast('✗ '+(r.error||'error'),true);
}

// ── Global time toggle ──
async function setGlobalTime(v){
  await api('/api/settings/time_global','POST',{enabled:v});
  toast(v?'✓ time packets ON':'✓ time packets OFF');
}

// ── Shared API ──
async function api(url,method,body){
  try{
    const r=await fetch(url,{method,headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    const j=await r.json();
    if(j.label&&j.hex)updLastBar(j.label,j.hex);
    return j;
  }catch(e){return{ok:false,error:e.message};}
}

// ── Toast ──
function toast(msg,err=false){
  const t=document.getElementById('toast');
  t.textContent=msg;t.className='show'+(err?' err':'');
  clearTimeout(t._t);t._t=setTimeout(()=>t.className='',2500);
}

// ── Last TX bar ──
function updLastBar(label,hex){
  document.getElementById('last-label').textContent=label;
  const h=document.getElementById('last-hex');
  h.textContent=(hex||'').match(/.{1,2}/g)?.join(' ')||'—';
  h.style.color='var(--accent)';
}

// ── Status poll ──
let agoBase=0,agoTimer;
async function poll(){
  try{
    const j=await(await fetch('/api/status')).json();
    if(j.label&&j.label!=='none')updLastBar(j.label,j.hex);
    if(j.ms_ago!==null){
      agoBase=j.ms_ago;
      clearInterval(agoTimer);
      function ua(){const s=Math.floor(agoBase/1000);agoBase+=2000;
        document.getElementById('last-ago').textContent=s+'s ago';}
      ua();agoTimer=setInterval(ua,2000);
    }
    if(typeof j.send_time_global!=='undefined')
      document.getElementById('global-time').checked=j.send_time_global;
  }catch(_){}
}
poll();setInterval(poll,10000);

// Init
updTDisp();

// ── TX Terminal ──
let logSince = 0;
let logEntries = [];   // newest-first display list
const termBody = document.getElementById('term-body');
const termCount = document.getElementById('term-count');

// Annotate known packet types from hex
function annotateHex(hex) {
  // Split into space-separated bytes for display
  const bytes = hex.match(/.{2}/g) || [];
  if (bytes.length !== 8) return hex;
  const b3 = parseInt(bytes[2],16);
  const b7 = parseInt(bytes[6],16) & 0x0f;
  const type = parseInt(bytes[6],16);

  let tag = '';
  if (bytes[0]==='d0'&&bytes[1]==='23') {
    if (type===0x01) tag = ' <span>// TIME HMS</span>';
    else if (type===0x02) tag = ' <span>// BLUE OFF (sched)</span>';
    else if (type>=0x03&&type<=0x05) tag = ' <span>// SCHED OFF</span>';
    else if (type>=0x08&&type<=0x09) tag = ' <span>// SCHED ON</span>';
    else if ((b3&0xf0)===0x80||(b3&0xf0)===0x00) tag = ' <span>// CMD</span>';
  }
  return bytes.join(' ') + tag;
}

function renderLog() {
  if (logEntries.length === 0) {
    termBody.innerHTML = '<div class="log-empty">No transmissions yet</div>';
    termCount.textContent = '0 entries';
    return;
  }

  let html = '';
  // logEntries is newest-first; display newest at bottom (reverse)
  [...logEntries].reverse().forEach(e => {
    const elapsed = Math.floor((Date.now() - (window._bootMs||0) + e.ms) / 1000);
    const mins = Math.floor(e.ms/60000);
    const secs = Math.floor((e.ms%60000)/1000);
    const ts = `+${String(mins).padStart(2,'0')}:${String(secs).padStart(2,'0')}`;

    html += `<div class="log-entry">`;
    html += `<div><span class="log-ts">${ts}</span><span class="log-label">${e.label}</span></div>`;
    (e.pkts||[]).forEach(p => {
      html += `<div class="log-pkt">`;
      html += `<span class="log-note">${p.note}</span>`;
      html += `<span class="log-hex">${annotateHex(p.hex)}</span>`;
      html += `</div>`;
    });
    html += `</div>`;
  });

  termBody.innerHTML = html;
  termCount.textContent = logEntries.length + ' entr' + (logEntries.length===1?'y':'ies');

  if (document.getElementById('auto-scroll').checked) {
    termBody.scrollTop = termBody.scrollHeight;
  }
}

async function pollLog() {
  try {
    const r = await fetch('/api/log?since='+logSince);
    const j = await r.json();
    if (j.entries && j.entries.length > 0) {
      // entries arrive newest-first; add to our list
      j.entries.forEach(e => {
        logEntries.unshift(e);  // prepend (keep newest-first)
      });
      // cap at 200 entries
      if (logEntries.length > 200) logEntries.length = 200;
      logSince = j.count;
      renderLog();
    }
  } catch(_) {}
}

function clearLog() {
  logEntries = [];
  logSince = 0;
  renderLog();
  // also reset server-side by just moving our since pointer up
  fetch('/api/log').then(r=>r.json()).then(j=>{ logSince=j.count; });
}

// ── On-load: populate UI from device state ──
async function loadDeviceState() {
  try {
    const s = await api('/api/status','GET');
    if(s.time){
      document.getElementById('t-hh').value = s.time.hh;
      document.getElementById('t-mm').value = s.time.mm;
      document.getElementById('t-ss').value = s.time.ss;
      updTDisp();
    }
  } catch(e){}
  try {
    const ch = await api('/api/channels','GET');
    CH.white.on=ch.white_on; setCh('white',ch.white_on);
    CH.white.level=ch.white_level;
    document.getElementById('sl-white').value=ch.white_level;
    document.getElementById('pct-white').value=ch.white_level*10;
    CH.blue.on=ch.blue_on; setCh('blue',ch.blue_on);
    CH.blue.level=ch.blue_level;
    document.getElementById('sl-blue').value=ch.blue_level;
    document.getElementById('pct-blue').value=ch.blue_level*10;
    CH.rgb.on=ch.rgb_on; setCh('rgb',ch.rgb_on);
    CH.rgb.level=ch.rgb_level;
    document.getElementById('sl-rgb').value=ch.rgb_level;
    document.getElementById('pct-rgb').value=ch.rgb_level*10;
    CH.rgb.color=ch.rgb_color;
    document.querySelectorAll('#color-pills .color-pill').forEach(p=>{
      p.classList.toggle('sel', parseInt(p.dataset.val)===ch.rgb_color);
    });
    CH.rgb.cycle=ch.rgb_cycle;
    if(ch.rgb_cycle>1){ const sel=document.getElementById('rgb-cycle'); if(sel) sel.value=ch.rgb_cycle; }
    updateRgbDurationRow();
    updateSchedStatePreviews();
  } catch(e){}
  try {
    const sc = await api('/api/schedule','GET');
    function fillSlot(pfx,slot){
      document.getElementById(pfx+'-en').checked=slot.active;
      document.getElementById(pfx+'-hh').value=slot.hh;
      document.getElementById(pfx+'-mm').value=slot.mm;
      schedRowToggle(pfx);
    }
    fillSlot('sw-on', sc.white_on);  fillSlot('sw-off',sc.white_off);
    fillSlot('sb-on', sc.blue_on);   fillSlot('sb-off',sc.blue_off);
    fillSlot('sr-on', sc.rgb_on);    fillSlot('sr-off',sc.rgb_off);
    if(sc.rgb_on.state)  schedRgbState['sr-on']  = sc.rgb_on.state;
    if(sc.rgb_off.state) schedRgbState['sr-off'] = sc.rgb_off.state;
    ['sr-on-pills','sr-off-pills'].forEach(id=>{
      const c=document.getElementById(id); if(!c) return;
      const cur=schedRgbState[id.replace('-pills','')]&0x0f;
      c.querySelectorAll('.color-pill').forEach(p=>p.classList.toggle('sel',parseInt(p.dataset.val)===cur));
    });
  } catch(e){}
}
loadDeviceState();

pollLog();
setInterval(pollLog, 1500);

</script>
</body>
</html>
)HTMLEOF";
