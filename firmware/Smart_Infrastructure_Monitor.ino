#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ===== PIN CONFIGURATION =====
// Hardware pin mapping and project thresholds.
// Confirm these pins against your actual ESP32 board and wiring.

#define TRIG_PIN   33   // Ultrasonic Trigger
#define ECHO_PIN   25   // Ultrasonic Echo
#define PIPE_PIN   15   // LM393 Drainage pipe sensor (DO pin -> Digital)
#define FLOW1_PIN  21   // Water flow sensor 1
#define FLOW2_PIN  27   // Water flow sensor 2
#define LED_PIN    18   // Street light LED
#define LDR_PIN    13   // LDR module DO -> GPIO13 (DIGITAL, WiFi-safe). Hardware fixed, no change needed.
// LDR module calibrated inverted: DO = HIGH means LIGHT, LOW means DARK.
#define LDR_LIGHT_LEVEL HIGH

#define LDR2_PIN   34   // Second LDR (analog) -> GPIO34 for Day/Night light level
#define LDR2_THRESHOLD 2000  // analog value above this = DAY (bright), below = NIGHT (dark)

#define MANHOLE_DEPTH_CM 22.0   // Demo: max distance 22cm
#define LEAK_DIFF_THRESHOLD 1.5  // L/min margin for demo (increase to 15 if needed)

// ===== WEB DASHBOARD HTML/CSS/JS =====
#pragma once
// Web dashboard HTML. Keep this file focused on the frontend only.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Infrastructure Monitor</title>
<style>
  :root{
    --bg:#0b1120;
    --card:#131b2d;
    --text:#e8eef7;
    --muted:#94a3b8;
    --primary:#3b82f6;
    --success:#22c55e;
    --warning:#f59e0b;
    --danger:#ef4444;
    --border:#23304a;
    --shadow:0 10px 28px rgba(0,0,0,.4);
  }
  *{margin:0;padding:0;box-sizing:border-box;font-family:Inter,'Segoe UI',Roboto,Helvetica,Arial,sans-serif}
  body{background:var(--bg);color:var(--text);min-height:100vh;line-height:1.55}
  header{background:linear-gradient(135deg,#0f172a 0%,#1e293b 100%);border-bottom:1px solid var(--border);padding:20px 24px;position:sticky;top:0;z-index:10;box-shadow:var(--shadow)}
  .header-inner{max-width:1200px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;gap:14px}
  .brand{display:flex;align-items:center;gap:12px}
  .brand .mark{width:36px;height:36px;border-radius:11px;background:linear-gradient(135deg,#3b82f6,#06b6d4);display:flex;align-items:center;justify-content:center;font-weight:800;color:#fff;font-size:17px}
  .brand h1{font-size:18px;font-weight:700;letter-spacing:.3px}
  .brand span{font-size:12px;color:var(--muted);display:block;margin-top:1px}
  .live{font-size:12px;font-variant-numeric:tabular-nums;color:var(--muted);background:rgba(30,41,59,.5);border:1px solid var(--border);border-radius:9px;padding:6px 13px}
  .ipbar{text-align:center;background:rgba(30,41,59,.45);border:1px solid var(--border);padding:9px 0;font-size:12px;color:var(--muted)}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:16px;padding:24px;max-width:1200px;margin:0 auto}
  .card{background:var(--card);border:1px solid var(--border);border-radius:16px;padding:20px;transition:transform .2s,border-color .2s,box-shadow .2s;display:flex;flex-direction:column;align-items:center}
  .card:hover{border-color:#334155;box-shadow:0 12px 32px rgba(0,0,0,.45)}
  .card-head{width:100%;display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
  .card-title{display:flex;align-items:center;gap:10px}
  .card h2{font-size:15px;font-weight:700;color:var(--text);letter-spacing:.25px}
  .tag{font-size:10px;font-weight:800;padding:3px 10px;border-radius:999px;background:rgba(59,130,246,.12);color:#93c5fd;border:1px solid rgba(59,130,246,.25);text-transform:uppercase;letter-spacing:.4px}
  .sub{font-size:11.5px;color:var(--muted);margin-bottom:14px;text-align:center;width:100%}
  .value{font-size:30px;font-weight:800;color:var(--text);font-variant-numeric:tabular-nums;margin:6px 0 2px}
  .unit{font-size:13px;color:var(--muted);font-weight:600}
  .badge{display:inline-flex;align-items:center;gap:8px;padding:7px 16px;border-radius:10px;font-size:12px;font-weight:800;letter-spacing:.4px;margin-top:8px}
  .bg-ok{background:rgba(34,197,94,.15);color:#4ade80;border:1px solid rgba(34,197,94,.35)}
  .bg-warn{background:rgba(245,158,11,.15);color:#fcd34d;border:1px solid rgba(245,158,11,.35)}
  .bg-danger{background:rgba(239,68,68,.15);color:#f87171;border:1px solid rgba(239,68,68,.35)}
  .bg-info{background:rgba(59,130,246,.15);color:#93c5fd;border:1px solid rgba(59,130,246,.35)}
  .bg-night{background:rgba(148,163,184,.15);color:#cbd5e1;border:1px solid rgba(148,163,184,.35)}
  .row{width:100%;display:flex;justify-content:space-between;background:#0f172a;border:1px solid var(--border);border-radius:10px;padding:9px 12px;margin:7px 0;font-size:12.5px}
  .row .lbl{color:var(--muted)}
  .row .val{font-weight:800;color:var(--text)}
  .switch{position:relative;width:56px;height:28px;display:inline-block}
  .switch input{display:none}
  .slider{position:absolute;inset:0;background:#334155;border-radius:28px;cursor:pointer;transition:.25s}
  .slider:before{content:"";position:absolute;width:20px;height:20px;background:#fff;border-radius:50%;top:4px;left:4px;transition:.25s}
  input:checked+.slider{background:var(--success)}
  input:checked+.slider:before{transform:translateX(28px)}
  .switch-row{display:flex;align-items:center;gap:11px;margin:10px 0;width:100%}
  .switch-row b{font-size:13px;color:var(--text)}
  footer{text-align:center;padding:16px;font-size:11.5px;color:var(--muted);border-top:1px solid var(--border)}
  .muted-note{font-size:11px;color:var(--muted);margin-top:8px;text-align:center;width:100%}

  /* ---------- Animated graphics ---------- */
  .graphic-wrap{width:100%;display:flex;flex-direction:column;align-items:center;gap:10px}

  /* Day/Night globe */
  .dn-stage{position:relative;width:110px;height:110px;display:flex;align-items:center;justify-content:center}
  .dn-globe{width:64px;height:64px;border-radius:50%;display:flex;align-items:center;justify-content:center;transition:background .4s,border-color .4s,box-shadow .4s}
  .dn-globe.day{background:#fbbf24;box-shadow:0 0 35px rgba(251,191,36,.6)}
  .dn-globe.night{background:#0f172a;border:2px solid #94a3b8;box-shadow:0 0 25px rgba(148,163,184,.25)}
  .dn-sun{width:30px;height:30px}
  .dn-moon{width:30px;height:30px}
  .dn-rays{position:absolute;inset:0;animation:rotate 6s linear infinite}
  .dn-ray{position:absolute;top:50%;left:50%;width:2px;height:14px;background:#fbbf24;transform-origin:center}
  @keyframes rotate{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}

  /* Manhole vertical pipe */
  .mh-stage{width:100px;height:170px;position:relative;display:flex;align-items:center;justify-content:center}
  .mh-pipe{width:44px;height:140px;background:#0f172a;border:3px solid #334155;border-radius:8px;position:relative;overflow:hidden;display:flex;align-items:flex-end;justify-content:center}
  .mh-water{width:100%;background:linear-gradient(180deg,#06b6d4,#3b82f6);position:absolute;bottom:0;transition:height 1s ease;box-shadow:0 0 20px rgba(6,182,212,.5)}
  .mh-cap{width:100%;height:8px;background:#334155;border-radius:4px 4px 0 0;position:absolute;top:0}
  .mh-drops{position:absolute;bottom:-26px;left:50%;transform:translateX(-50%);display:flex;gap:5px}
  .drop{width:6px;height:10px;background:#3b82f6;border-radius:50% 50% 50% 50% / 60% 60% 40% 40%;opacity:0;animation:dropFall 1.2s ease-in-out infinite}
  .drop:nth-child(2){animation-delay:.25s}
  .drop:nth-child(3){animation-delay:.5s}
  @keyframes dropFall{0%{transform:translateY(0);opacity:0}40%{opacity:1}100%{transform:translateY(22px);opacity:0}}

  /* Drainage horizontal pipe */
  .pipe-stage{width:160px;height:90px;position:relative;display:flex;align-items:center;justify-content:center}
  .h-pipe{width:130px;height:22px;background:#1f2a44;border:3px solid #334155;border-radius:6px;position:relative;overflow:hidden}
  .h-flow{position:absolute;left:0;top:0;height:100%;background:linear-gradient(90deg,#06b6d4,#3b82f6);width:0}
  .h-full{position:absolute;left:0;top:0;height:100%;background:#ef4444;width:0;box-shadow:0 0 20px rgba(239,68,68,.5)}
  .h-cap{position:absolute;left:0;top:-4px;width:10px;height:30px;background:#1f2a44;border:3px solid #334155;border-radius:6px 0 0 6px;border-right:none}
  .h-cap-r{position:absolute;right:0;top:-4px;width:10px;height:30px;background:#1f2a44;border:3px solid #334155;border-radius:0 6px 6px 0;border-left:none}
  .h-dot{position:absolute;top:50%;transform:translateY(-50%);width:6px;height:6px;background:#fff;border-radius:50%;box-shadow:0 0 8px #fff;opacity:0;animation:pipeFlow 1.4s linear infinite}
  @keyframes pipeFlow{0%{left:-8px;opacity:0}20%{opacity:1}80%{opacity:1}100%{left:138px;opacity:0}}

  /* Street light bulb */
  .bulb-stage{width:110px;height:160px;position:relative;display:flex;flex-direction:column;align-items:center;gap:8px}
  .bulb{width:52px;height:64px;background:#0f172a;border:4px solid #334155;border-radius:52% 52% 42% 42%/62% 62% 38% 38%;position:relative;transition:box-shadow .3s,background .3s}
  .bulb.on{box-shadow:0 0 45px rgba(251,191,36,.9), inset 0 0 18px rgba(251,191,36,.35);background:#fef3c7}
  .bulb-filament{position:absolute;top:32%;left:50%;transform:translateX(-50%);width:14px;height:14px;border:2px solid #94a3b8;border-radius:50%}
  .bulb.on .bulb-filament{border-color:#f59e0b;box-shadow:0 0 14px rgba(245,158,11,.9)}
  .bulb-base{position:absolute;bottom:-15px;left:50%;transform:translateX(-50%);width:24px;height:14px;background:#334155;border-radius:4px}
  .bulb-glow{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);width:110px;height:110px;background:radial-gradient(circle,rgba(251,191,36,.4),transparent 70%);border-radius:50%;opacity:0;transition:opacity .3s}
  .bulb.on .bulb-glow{opacity:1;animation:bulbPulse 3s ease-in-out infinite}
  .bulb-rays{position:absolute;inset:-12px;filter:blur(10px);opacity:0;transition:opacity .3s}
  .bulb.on .bulb-rays{opacity:.7}
  @keyframes bulbPulse{0%,100%{transform:translate(-50%,-50%) scale(1);opacity:.55}50%{transform:translate(-50%,-50%) scale(1.18);opacity:.9}}

  @media (max-width:640px){.grid{padding:16px}.header-inner{flex-direction:column;align-items:flex-start;gap:10px}}
</style>
</head>
<body>
<header>
  <div class="header-inner">
    <div class="brand">
      <div class="mark">SI</div>
      <div>
        <h1>Smart Infrastructure Monitor</h1>
        <span>ESP32 &bull; Sewerage &bull; Water &bull; Street Light &bull; Day/Night</span>
      </div>
    </div>
    <div class="live" id="clock">--:--:--</div>
  </div>
</header>
<div class="ipbar" id="ip">Connecting to WiFi...</div>

<div class="grid">
  <!-- Ambient Light / Day-Night -->
  <div class="card" style="grid-column:1/-1;flex-direction:row;justify-content:space-between;align-items:center">
    <div style="display:flex;flex-direction:column;align-items:center;gap:10px;flex:1">
      <div class="dn-stage">
        <div class="dn-globe day" id="dnGlobe">
          <div class="dn-rays" id="dnRays"></div>
          <svg class="dn-sun" viewBox="0 0 24 24" aria-hidden="true">
            <circle cx="12" cy="12" r="3" fill="currentColor"/>
            <path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M17.66 6.34l1.41-1.41" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" fill="none"/>
          </svg>
          <svg class="dn-moon" viewBox="0 0 24 24" aria-hidden="true" style="display:none">
            <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" fill="currentColor"/>
          </svg>
        </div>
        <div class="value" id="dnLabel">--</div>
        <div class="unit" id="dnSub">Light level: <b id="ldr2Val">--</b>%</div>
      </div>
      <div style="width:100%;max-width:220px">
        <div class="bar"><div id="dnBar" style="width:50%"></div></div>
      </div>
      <span class="badge" id="dnBadge" style="margin-top:6px">--</span>
    </div>
    <div class="tag bg-info" style="align-self:flex-start">LDR #2 &bull; GPIO34</div>
  </div>

  <!-- Manhole vertical pipe -->
  <div class="card">
    <div class="card-head">
      <div class="card-title">
        <svg class="icon icon-manhole" viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="12" cy="12" r="9" stroke="currentColor" stroke-width="2" fill="none"/>
          <circle cx="12" cy="12" r="3" fill="currentColor"/>
          <path d="M5 12h14M12 5v14" stroke="currentColor" stroke-width="1.5" opacity=".5"/>
        </svg>
        <h2>Manhole Level</h2>
      </div>
      <span class="tag bg-info">Ultrasonic</span>
    </div>
    <div class="sub">TRIG:33 &bull; ECHO:25 &bull; Depth 22 cm</div>
    <div class="graphic-wrap">
      <div class="mh-stage">
        <div class="mh-pipe">
          <div class="mh-water" id="mhWater" style="height:0%"></div>
          <div class="mh-cap"></div>
        </div>
        <div class="mh-drops" id="mhDrops">
          <div class="drop"></div><div class="drop"></div><div class="drop"></div>
        </div>
      </div>
      <div class="value"><span id="mcm">--</span> <span class="unit">cm gap</span></div>
      <div class="row"><span class="lbl">Filled</span><b id="mpct">-- %</b></div>
    </div>
    <span class="badge bg-info" id="mstatus">LOADING</span>
  </div>

  <!-- Drainage horizontal pipe -->
  <div class="card">
    <div class="card-head">
      <div class="card-title">
        <svg class="icon icon-pipe" viewBox="0 0 24 24" aria-hidden="true">
          <path d="M4 12h16" stroke="currentColor" stroke-width="4" stroke-linecap="round" fill="none"/>
          <path d="M4 10h2M18 10h2M4 14h2M18 14h2" stroke="currentColor" stroke-width="2" fill="none"/>
          <path d="M10 8l2 4 2-4" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
        </svg>
        <h2>Drainage Pipe</h2>
      </div>
      <span class="tag bg-info">LM393 &bull; GPIO15</span>
    </div>
    <div class="sub">LOW = FULL &bull; HIGH = EMPTY</div>
    <div class="graphic-wrap">
      <div class="pipe-stage">
        <div class="h-pipe">
          <div class="h-flow" id="hFlow" style="width:100%"></div>
          <div class="h-full" id="hFull" style="width:0%"></div>
          <div class="h-dot" id="pDot1" style="left:-8px;animation-delay:0s"></div>
          <div class="h-dot" id="pDot2" style="left:-8px;animation-delay:.45s"></div>
          <div class="h-dot" id="pDot3" style="left:-8px;animation-delay:.9s"></div>
        </div>
        <div class="h-cap"></div>
        <div class="h-cap-r"></div>
      </div>
      <div class="value" id="pipeText">--</div>
    </div>
    <span class="badge bg-info" id="pipeBadge">CHECKING</span>
    <div class="row"><span class="lbl">Sensor pin</span><span class="val">GPIO 15</span></div>
  </div>

  <!-- Water leakage -->
  <div class="card">
    <div class="card-head">
      <div class="card-title">
        <svg class="icon icon-water" viewBox="0 0 24 24" aria-hidden="true">
          <path d="M12 2.69l3.91 5.88a1 1 0 0 0 1.5 0l3.91-5.88a1 1 0 0 1 1.5 0L19 14a1 1 0 0 1-1 1v4a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1v-4a1 1 0 0 1 1-1l2.39-3.57a1 1 0 0 0 1.5-0zM12 13a1 1 0 1 0 0-2 1 1 0 0 0 0 2z" fill="currentColor"/>
        </svg>
        <h2>Pipeline Leakage</h2>
      </div>
      <span class="tag bg-info">Dual Flow Sensors</span>
    </div>
    <div class="sub">Flow 1: GPIO21 &bull; Flow 2: GPIO27</div>
    <div class="row"><span class="lbl">Flow 1 (Point A)</span><b><span id="f1">--</span> L/min</b></div>
    <div class="row"><span class="lbl">Flow 2 (Point B)</span><b><span id="f2">--</span> L/min</b></div>
    <div class="row"><span class="lbl">Difference</span><b><span id="fdiff">--</span> L/min</b></div>
    <span class="badge bg-ok" id="leakBadge">NO LEAK - NORMAL</span>
  </div>

  <!-- Street light with bulb -->
  <div class="card">
    <div class="card-head">
      <div class="card-title">
        <svg class="icon icon-light" viewBox="0 0 24 24" aria-hidden="true">
          <path d="M12 2a1 1 0 0 1 1 1v4h2a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1H12a1 1 0 0 1-1-1V7a1 1 0 0 1 1-1V3a1 1 0 0 1 1-1zM12 17a1 1 0 1 0 0 2 1 1 0 0 0 0-2z" fill="currentColor"/>
          <circle cx="13" cy="7" r="2" fill="currentColor" opacity=".8"/>
        </svg>
        <h2>Street Light Control</h2>
      </div>
      <span class="tag bg-info">LED &bull; GPIO18</span>
    </div>
    <div class="row"><span class="lbl">Control Mode</span><b id="ledMode">AUTOMATIC</b></div>
    <div class="row"><span class="lbl">Day/Night Control</span><b id="ledLabel">Waiting for LDR...</b></div>
    <div class="graphic-wrap">
      <div class="bulb-stage">
        <div class="bulb" id="bulb">
          <div class="bulb-glow" id="bulbGlow"></div>
          <div class="bulb-rays" id="bulbRays"></div>
          <div class="bulb-filament" id="bulbFilament"></div>
        </div>
        <div class="bulb-base"></div>
        <div class="value" id="bulbLabel" style="font-size:16px">Light OFF</div>
      </div>
    </div>
    <div class="row"><span class="lbl">LDR #1 Digital (GPIO13)</span><b id="ldr">--</b></div>
    <div class="row"><span class="lbl">LDR #1 State</span><b id="lpct">--</b></div>
    <span class="badge bg-info" id="streetBadge" style="display:none">--</span>
    <div class="muted-note">LDR #2: Day = LED OFF, Night = LED ON. LDR #1 checks whether the street light is illuminated at night.</div>
  </div>
</div>

<footer>Auto-refresh every 2 s &bull; ESP32 Station Mode (Home WiFi)</footer>

<script>
function pad(n){return n<10?'0'+n:n}
function clock(){
  var now=new Date();
  document.getElementById('clock').innerText=pad(now.getHours())+':'+pad(now.getMinutes())+':'+pad(now.getSeconds());
}
setInterval(clock,1000);clock();
function update(){
  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('ip').innerText='ESP32 IP: '+d.ip+'  |  WiFi: '+d.ssid+'  |  Uptime: '+d.time;
    // manhole vertical pipe
    var pct = Math.max(0,Math.min(100,d.manhole_pct));
    document.getElementById('mcm').innerText=d.manhole_cm<0?'ERR':d.manhole_cm.toFixed(1);
    document.getElementById('mpct').innerText=pct.toFixed(0)+' %';
    document.getElementById('mhWater').style.height=pct+'%';
    var drops=document.getElementById('mhDrops');
    drops.style.display = pct>=50?'flex':'none';
    var ms=document.getElementById('mstatus');
    ms.innerText=d.manhole_status;
    ms.className='badge '+(d.manhole_pct>=50?'bg-danger':d.manhole_pct>=60?'bg-warn':d.manhole_pct>=30?'bg-warn':'bg-ok');
    // drainage horizontal pipe
    var full=d.pipe_full==1;
    document.getElementById('pipeText').innerText=full?'PIPE FULL':'PIPE EMPTY';
    var hFlow=document.getElementById('hFlow');
    var hFull=document.getElementById('hFull');
    var dots=[document.getElementById('pDot1'),document.getElementById('pDot2'),document.getElementById('pDot3')];
    if(full){
      hFlow.style.width='0%';
      hFull.style.width='100%';
      dots.forEach(function(dot){dot.style.opacity='0'});
    } else {
      hFlow.style.width='100%';
      hFull.style.width='0%';
      dots.forEach(function(dot){dot.style.opacity='1'});
    }
    var pb=document.getElementById('pipeBadge');
    pb.innerText=full?'PIPE FULL - NEED CLEANING':'PIPE EMPTY - FLOWING';
    pb.className='badge '+(full?'bg-danger':'bg-ok');
    // flow
    document.getElementById('f1').innerText=d.flow1.toFixed(2);
    document.getElementById('f2').innerText=d.flow2.toFixed(2);
    document.getElementById('fdiff').innerText=Math.abs(d.flow1-d.flow2).toFixed(2);
    var lb=document.getElementById('leakBadge');
    lb.innerText=d.leak==1?'LEAKAGE DETECTED':'NO LEAK - NORMAL';
    lb.className='badge '+(d.leak==1?'bg-danger':'bg-ok');
    // street light bulb
    document.getElementById('ledMode').innerText='AUTOMATIC';
    document.getElementById('ledLabel').innerText=d.day_night==='NIGHT'?'NIGHT - STREET LIGHT ON':'DAY - STREET LIGHT OFF';
    var bulb=document.getElementById('bulb');
    var bulbLabel=document.getElementById('bulbLabel');
    if(d.led==1){
      bulb.className='bulb on';
      bulbLabel.innerText='NIGHT - LIGHT ON';
    } else {
      bulb.className='bulb';
      bulbLabel.innerText='DAY - LIGHT OFF';
    }
    document.getElementById('ldr').innerText=(d.ldr==0?'LOW (0)':'HIGH (1)');
    document.getElementById('lpct').innerText=d.ldr_state;
    var sb=document.getElementById('streetBadge');
    if(!d.street_status){sb.style.display='none'}else{
      sb.style.display='inline-block';
      sb.innerText=d.street_status;
      sb.className='badge '+(d.street_status.indexOf('GOOD')>=0?'bg-ok':d.street_status.indexOf('DAMAGED')>=0?'bg-danger':'bg-info');
    }
    // day / night sun/moon
    var isDay=d.day_night==='DAY';
    document.getElementById('dnLabel').innerText=d.day_night;
    document.getElementById('dnSub').innerHTML='Light level: <b id="ldr2Val">'+d.ldr2_pct.toFixed(1)+'</b>%';
    document.getElementById('dnBar').style.width=Math.max(0,Math.min(100,d.ldr2_pct))+'%';
    var dnb=document.getElementById('dnBadge');
    var globe=document.getElementById('dnGlobe');
    var rays=document.getElementById('dnRays');
    var sun=globe.querySelector('.dn-sun');
    var moon=globe.querySelector('.dn-moon');
    if(isDay){
      dnb.innerText='DAYTIME - BRIGHT';
      dnb.className='badge bg-ok';
      globe.className='dn-globe day';
      rays.style.display='block';
      if(sun)sun.style.display='block';
      if(moon)moon.style.display='none';
    } else {
      dnb.innerText='NIGHTTIME - DARK';
      dnb.className='badge bg-night';
      globe.className='dn-globe night';
      rays.style.display='none';
      if(sun)sun.style.display='none';
      if(moon)moon.style.display='block';
    }
  }).catch(e=>{});
}
setInterval(update,2000);
update();
</script>
</body>
</html>
)rawliteral";

 
// ============ HOME WIFI (STA MODE - NO ACCESS POINT) ============
// Change these to your home WiFi router credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ===== CIVICFIX CLOUD AUTO-COMPLAINT SETTINGS =====
// Replace these values before uploading. Use the same token in Render's ESP32_DEVICE_TOKEN.
const char* CIVICFIX_API_URL = "https://civicfix-smart-monitor-1.onrender.com/api/esp32/telemetry";
const char* CIVICFIX_DEVICE_TOKEN = "CHANGE_TO_MATCH_RENDER_TOKEN";
const char* CIVICFIX_DEVICE_NAME = "Infrastructure Node 01";
const char* CIVICFIX_SITE_ADDRESS = "Prof. Ram Meghe Institute of Technology & Research, Badnera, Amravati, Maharashtra 444701";
// Set coordinates for the actual pole/manhole/pipe installation. 0,0 means not configured.
const double CIVICFIX_SITE_LATITUDE = 20.8542175212946;
const double CIVICFIX_SITE_LONGITUDE = 77.75135521911784;
const unsigned long CIVICFIX_POST_INTERVAL_MS = 10000;
unsigned long lastCivicFixPost = 0;

WebServer server(80);

// Digital LDR: returns true = LIGHT, false = DARK
bool readLDR_Light() {
  return digitalRead(LDR_PIN) == LDR_LIGHT_LEVEL;
}

// Second LDR (analog): returns raw 0-4095, higher = brighter
int readLDR2_Raw() {
  return analogRead(LDR2_PIN);
}

volatile unsigned long flow1Pulses = 0;
volatile unsigned long flow2Pulses = 0;

void IRAM_ATTR flow1_ISR() { flow1Pulses++; }
void IRAM_ATTR flow2_ISR() { flow2Pulses++; }

bool ledState = false;
float flow1Rate = 0, flow2Rate = 0;
int ldr2Raw = 0;
float ldr2Pct = 0;
String dayNight = "DAY";
unsigned long lastCalc = 0;
unsigned long lastCount1 = 0, lastCount2 = 0;

// ---------- Ultrasonic ----------
float readManholeDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return -1; // no echo
  return (duration * 0.0343) / 2.0; // cm
}

String getManholeStatus(float fillPct) {
  if (fillPct < 0) return "SENSOR ERROR";
  if (fillPct >= 50) return "HALF-FULL ALERT";
  if (fillPct >= 60) return "FILLED";
  if (fillPct >= 30) return "HALF FILLED";
  return "NORMAL";
}

// ---------- HTML DASHBOARD ----------


// ---------- Web handlers ----------
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}


void handleData() {
  float dist = readManholeDistanceCM();
  if (dist > MANHOLE_DEPTH_CM) dist = MANHOLE_DEPTH_CM; // cap demo max at 22cm
  float fillPct = 0;
  if (dist < 0) fillPct = 0;
  else {
    fillPct = ((MANHOLE_DEPTH_CM - dist) / MANHOLE_DEPTH_CM) * 100.0;
    if (fillPct < 0) fillPct = 0;
    if (fillPct > 100) fillPct = 100;
  }
  String mStatus = getManholeStatus(dist < 0 ? -1 : fillPct);

  int pipeVal = digitalRead(PIPE_PIN); // LM393 DO: LOW = water present (FULL) - reversed logic
  int pipeFull = (pipeVal == LOW) ? 1 : 0;

  int ldrRaw = digitalRead(LDR_PIN);          // 0 or 1, WiFi-safe on GPIO13
  bool isLight = (ldrRaw == LDR_LIGHT_LEVEL); // true = LIGHT, false = DARK
  String ldrState = isLight ? "LIGHT" : "DARK";

  // LDR #2 (GPIO34) determines day/night and automatically controls the street light.
  // Bright = DAY (LED OFF); dark = NIGHT (LED ON).
  ldr2Raw = readLDR2_Raw();
  ldr2Pct = (ldr2Raw / 4095.0) * 100.0;
  dayNight = (ldr2Raw > LDR2_THRESHOLD) ? "DAY" : "NIGHT";
  ledState = (dayNight == "NIGHT");
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  String street;
  if (dayNight == "DAY") street = "DAYTIME - STREET LIGHT OFF";
  else if (isLight) street = "STREET LIGHT WORKING GOOD";
  else street = "STREET LIGHT DAMAGED";

  int leak = (abs(flow1Rate - flow2Rate) > LEAK_DIFF_THRESHOLD && (flow1Rate > 0.3 || flow2Rate > 0.3)) ? 1 : 0;

  String json = "{";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"ssid\":\"" + String(ssid) + "\",";
  json += "\"time\":\"" + String(millis()/1000) + "s uptime\",";
  json += "\"manhole_cm\":" + String(dist,1) + ",";
  json += "\"manhole_pct\":" + String(fillPct,1) + ",";
  json += "\"manhole_status\":\"" + mStatus + "\",";
  json += "\"pipe_full\":" + String(pipeFull) + ",";
  json += "\"flow1\":" + String(flow1Rate,2) + ",";
  json += "\"flow2\":" + String(flow2Rate,2) + ",";
  json += "\"leak\":" + String(leak) + ",";
  json += "\"led\":" + String(ledState?1:0) + ",";
  json += "\"ldr\":" + String(ldrRaw) + ",";
  json += "\"ldr_state\":\"" + ldrState + "\",";
  json += "\"light_pct\":" + String(isLight ? 100 : 0) + ",";
  json += "\"ldr2\":" + String(ldr2Raw) + ",";
  json += "\"ldr2_pct\":" + String(ldr2Pct,1) + ",";
  json += "\"day_night\":\"" + dayNight + "\",";
  json += "\"street_status\":\"" + street + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Send sensor state to CivicFix independently of the local dashboard being open.
// setInsecure() is convenient for a student prototype but skips certificate validation.
// For production, validate the server certificate or use a trusted CA certificate.
void postCivicFixTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (String(CIVICFIX_API_URL).indexOf("YOUR-APP") >= 0 || String(CIVICFIX_DEVICE_TOKEN) == "CHANGE_TO_MATCH_RENDER_TOKEN") return;

  float dist = readManholeDistanceCM();
  float fillPct = 0;
  if (dist >= 0) {
    if (dist > MANHOLE_DEPTH_CM) dist = MANHOLE_DEPTH_CM;
    fillPct = ((MANHOLE_DEPTH_CM - dist) / MANHOLE_DEPTH_CM) * 100.0;
    fillPct = constrain(fillPct, 0.0f, 100.0f);
  }
  int pipeFull = digitalRead(PIPE_PIN) == LOW ? 1 : 0;
  int ldrRaw = digitalRead(LDR_PIN);
  bool isLight = (ldrRaw == LDR_LIGHT_LEVEL);
  ldr2Raw = analogRead(LDR2_PIN);
  dayNight = (ldr2Raw > LDR2_THRESHOLD) ? "DAY" : "NIGHT";
  ledState = (dayNight == "NIGHT");
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  int leak = (abs(flow1Rate - flow2Rate) > LEAK_DIFF_THRESHOLD && (flow1Rate > 0.3 || flow2Rate > 0.3)) ? 1 : 0;
  bool streetBroken = (dayNight == "NIGHT" && ledState && !isLight);
  bool manholeDanger = (dist >= 0 && fillPct >= 50.0);

  String body = "{";
  body += "\"device_name\":\"" + String(CIVICFIX_DEVICE_NAME) + "\",";
  body += "\"address\":\"" + String(CIVICFIX_SITE_ADDRESS) + "\",";
  body += "\"latitude\":" + String(CIVICFIX_SITE_LATITUDE, 6) + ",";
  body += "\"longitude\":" + String(CIVICFIX_SITE_LONGITUDE, 6) + ",";
  body += "\"readings\":{";
  body += "\"manhole_pct\":" + String(fillPct, 1) + ",";
  body += "\"manhole_distance_cm\":" + String(dist, 1) + ",";
  body += "\"pipe_full\":" + String(pipeFull) + ",";
  body += "\"flow1_lpm\":" + String(flow1Rate, 2) + ",";
  body += "\"flow2_lpm\":" + String(flow2Rate, 2) + ",";
  body += "\"flow_difference_lpm\":" + String(abs(flow1Rate-flow2Rate), 2) + ",";
  body += "\"day_night\":\"" + dayNight + "\",";
  body += "\"street_light_on\":" + String(ledState ? "true" : "false") + ",";
  body += "\"ldr1_state\":\"" + String(isLight ? "LIGHT" : "DARK") + "\"},";
  body += "\"alerts\":[";
  body += "{\"key\":\"street_light_broken\",\"active\":" + String(streetBroken ? "true" : "false") + "},";
  body += "{\"key\":\"manhole_overflow_risk\",\"active\":" + String(manholeDanger ? "true" : "false") + "},";
  body += "{\"key\":\"drainage_pipe_full\",\"active\":" + String(pipeFull ? "true" : "false") + "},";
  body += "{\"key\":\"pipeline_leakage\",\"active\":" + String(leak ? "true" : "false") + "}";
  body += "]}";

  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // prototype only; validate TLS certificate in production
  HTTPClient http;
  http.setTimeout(5000);
  if (http.begin(secureClient, CIVICFIX_API_URL)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Token", CIVICFIX_DEVICE_TOKEN);
    int code = http.POST(body);
    Serial.printf("CivicFix telemetry HTTP: %d\n", code);
    if (code > 0) Serial.println(http.getString());
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LDR_PIN, INPUT);
  pinMode(LDR2_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIPE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(FLOW1_PIN, INPUT_PULLUP);
  pinMode(FLOW2_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW1_PIN), flow1_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW2_PIN), flow2_ISR, RISING);

  // --- STA MODE ONLY (connect to home WiFi, NO Access Point) ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Home WiFi: ");
  Serial.println(ssid);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 40) {
    delay(500);
    Serial.print(".");
    t++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("Dashboard URL: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed! Check SSID/PWD.");
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (millis() - lastCivicFixPost >= CIVICFIX_POST_INTERVAL_MS) {
    lastCivicFixPost = millis();
    postCivicFixTelemetry();
  }

  // Debug LDR + LED every 2 sec (open Serial Monitor to verify sensor)
  static unsigned long dbg = 0;
  if (millis() - dbg > 2000) {
    dbg = millis();
    int dbgRaw = digitalRead(LDR_PIN);
    bool dbgLight = (dbgRaw == LDR_LIGHT_LEVEL);
    int dbgRaw2 = analogRead(LDR2_PIN);
    Serial.printf("LED:%d LDR1_DO:%d (%s) | LDR2_ANALOG:%d (%.1f%%) [%s] => %s\n", ledState, dbgRaw,
      dbgLight ? "LIGHT" : "DARK", dbgRaw2, (dbgRaw2 / 4095.0) * 100.0, dayNight.c_str(),
      !ledState ? "OFF-no result" : (dbgLight ? "WORKING GOOD" : "DAMAGED"));
  }

  // Calculate flow rate every 1 sec: Freq/7.5 = L/min (YF-S201)
  if (millis() - lastCalc >= 1000) {
    noInterrupts();
    unsigned long c1 = flow1Pulses;
    unsigned long c2 = flow2Pulses;
    interrupts();
    unsigned long d1 = c1 - lastCount1;
    unsigned long d2 = c2 - lastCount2;
    lastCount1 = c1;
    lastCount2 = c2;
    lastCalc = millis();
    flow1Rate = d1 / 7.5;
    flow2Rate = d2 / 7.5;
  }
}