// Webseiten der Abfahrtstafel (werden direkt vom ESP32 ausgeliefert)
#pragma once

const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Abfahrtstafel</title><style>
*{box-sizing:border-box}
body{margin:0;font:16px/1.4 -apple-system,system-ui,sans-serif;background:#f2f3f8;color:#1b1f3a}
header{background:#1a2ab0;color:#fff;padding:18px 16px}
h1{margin:0;font-size:22px;max-width:520px;margin:auto}
main{padding:4px 16px 40px;max-width:520px;margin:auto}
section{background:#fff;border-radius:12px;padding:14px;margin:12px 0;box-shadow:0 1px 3px #0002}
h2{font-size:13px;margin:0 0 10px;color:#667;text-transform:uppercase;letter-spacing:.05em}
input,select{width:100%;font:inherit;padding:10px 12px;border:1px solid #ccd;border-radius:8px;background:#fff;color:inherit}
button{font:inherit;font-weight:600;padding:12px 16px;border:0;border-radius:8px;background:#1a2ab0;color:#fff;width:100%;margin-top:10px;cursor:pointer}
button:disabled{opacity:.4}
button.sec{background:#e6e8f5;color:#1a2ab0}
button.danger{color:#c00}
label{display:block;margin:12px 0 4px;font-weight:600}
.muted{color:#778;font-size:14px;margin:4px 0}
.st{border-top:1px solid #eee;padding:12px 0}.st:first-child{border:0;padding-top:0}
.row{display:flex;gap:8px}.row button{width:auto;margin:0;padding:0 14px}
.lines{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}
.chip{width:auto;margin:0;padding:6px 14px;border-radius:16px;background:#e6e8f5;color:#333}
.chip.on{background:#1a2ab0;color:#fff}
.grid{display:grid;grid-template-columns:1fr 72px;gap:8px 12px;align-items:center}
.grid label{margin:0;font-weight:500}
.grid input{padding:2px;height:40px}
.addr{display:flex;gap:8px;align-items:center;margin:8px 0}.addr a{flex:1;font-weight:600;color:#1a2ab0;word-break:break-all}.addr button{width:auto;margin:0;padding:8px 12px}
.chk{display:flex;gap:8px;align-items:center;font-weight:400}.chk input{width:auto}
#msg{text-align:center;min-height:1.4em;font-weight:600;margin:8px 0 0}
.seg{display:flex;background:#e6e8f5;border-radius:8px;padding:3px;gap:3px;flex:1}
.seg button{margin:0;padding:8px;background:transparent;color:#445;font-weight:600}
.seg button.on{background:#fff;color:#1a2ab0;box-shadow:0 1px 2px #0002}
.st .row{margin-bottom:8px}.st input{margin-bottom:8px}
.walk{display:flex;align-items:center;gap:8px;margin-top:12px;padding-top:10px;border-top:1px dashed #e3e5ef;color:#556;font-size:15px}.walk input{width:70px;margin:0;text-align:center}
.presets{display:flex;gap:8px}.presets button{margin:0;padding:10px 6px;font-size:14px;border:2px solid transparent}
.presets button.on{border-color:#1a2ab0}
details{margin-top:12px}summary{cursor:pointer;color:#1a2ab0;font-weight:600}
.times{display:flex;gap:8px;align-items:center;margin-top:8px}.times input{margin:0}
.bar{position:sticky;bottom:0;padding:8px 0 12px;background:linear-gradient(#f2f3f800,#f2f3f8 30%)}
.two{display:flex;gap:8px}.two button{margin:0;flex:1}
button.warn{background:#c00;color:#fff}
</style></head><body><header><h1>Abfahrtstafel</h1></header><main>)HTML";

// ---------- Einrichtung (eigenes WLAN der Tafel) ----------
const char PORTAL_HTML[] PROGMEM = R"HTML(
<section><h2>WLAN einrichten</h2>
<form method="post" action="/save">
<label for="s">WLAN</label><select id="s" name="ssid"><option>Suche…</option></select>
<input id="o" name="other" placeholder="Name des WLANs" style="display:none;margin-top:8px">
<button type="button" class="sec" id="r">Neu suchen</button>
<label for="p">Passwort</label><input id="p" name="pass" type="password" autocomplete="off">
<label class="chk"><input type="checkbox" onclick="p.type=this.checked?'text':'password'"> Passwort anzeigen</label>
<button>Verbinden</button>
</form></section>
<p class="muted">Danach verbindet sich die Tafel mit deinem WLAN. Stationen und Farben stellst du anschliessend über den QR-Code auf der Tafel ein.</p>
<script>
const s=document.getElementById('s'),o=document.getElementById('o');
async function load(re){
  s.innerHTML='<option>Suche…</option>';
  let l=[];try{l=await(await fetch('/api/networks'+(re?'?rescan=1':''))).json()}catch(e){}
  s.innerHTML='';
  for(const n of l)s.add(new Option(n.ssid+(n.lock?' 🔒':''),n.ssid));
  s.add(new Option('Anderes WLAN…','__other'));
  s.onchange();
}
s.onchange=()=>o.style.display=s.value=='__other'?'block':'none';
document.getElementById('r').onclick=()=>load(1);
load(0);
</script></main></body></html>)HTML";

const char SAVED_HTML[] PROGMEM = R"HTML(
<section><h2>Gespeichert</h2>
<p>Die Tafel startet neu und verbindet sich mit deinem WLAN.</p>
<p class="muted">Du kannst dieses Fenster schliessen. Klappt die Verbindung nicht, erscheint wieder der Einrichtungs-Code.</p>
</section></main></body></html>)HTML";

// ---------- Einstellungen (im Heim-WLAN) ----------
const char CONFIG_HTML[] PROGMEM = R"HTML(
<section><h2>Adresse dieser Seite</h2>
<div class="addr"><a id="url"></a><button type="button" class="sec" id="cp">Kopieren</button></div></section>

<section><h2>Titel</h2><input id="title" maxlength="24"></section>

<section><h2>Abfahrten</h2>
<div id="sts"></div>
<button type="button" class="sec" id="add">+ Hinzufügen</button></section>

<section><h2>Nachtmodus</h2>
<label class="chk"><input type="checkbox" id="non"> Anzeige nachts ausschalten</label>
<div class="times" id="nt"><input type="time" id="nfrom"><span>bis</span><input type="time" id="nto"></div></section>

<section><h2>Aussehen</h2>
<label>Schrift</label><div class="seg" id="font"><button type="button" data-f="sans">Normal</button><button type="button" data-f="pixel">Pixel (LED)</button></div>
<label>Farben</label><div class="presets" id="pre"></div>
<details><summary>Eigene Farben</summary><div id="cols" class="grid" style="margin-top:10px"></div></details></section>

<div class="bar"><button id="save">Speichern</button><p id="msg"></p></div>

<section><h2>Gerät</h2><div class="two">
<button type="button" class="sec" id="wr">WLAN ändern</button>
<button type="button" class="sec danger" id="fr">Werkseinstellungen</button></div>
<p class="muted" id="ver" style="text-align:center;margin-top:10px"></p></section>
<datalist id="dl"></datalist>
<script>
const $=i=>document.getElementById(i);
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const API="https://transport.opendata.ch/v1/";
const PLAIN=['T','B','NFT','NFB','TRAM','BUS',''];
const PRESETS={
  'SBB-Blau':{bg:'#1a2ab0',hdr:'#101878',line:'#4055d8',text:'#ffffff',delay:'#ffde15',muted:'#b4b8e6',go:'#ff4a3d'},
  'LED-Tafel':{bg:'#000000',hdr:'#000000',line:'#2a2a2a',text:'#ffb000',delay:'#ff5a00',muted:'#8a6a20',go:'#ff2a2a'},
  'Dunkel':{bg:'#15171c',hdr:'#0b0c0f',line:'#2c303a',text:'#e8e8ea',delay:'#ffcc33',muted:'#8b90a0',go:'#ff5a4d'}};
let D,font="sans";
function setFont(f){font=f;document.querySelectorAll("#font button").forEach(b=>b.classList.toggle("on",b.dataset.f===f))}
document.querySelectorAll("#font button").forEach(b=>b.onclick=()=>setFont(b.dataset.f));

function upd(){$('add').disabled=$('sts').children.length>=4}

// Vorschläge beim Tippen (Handy fragt den Fahrplan-Server direkt)
function suggest(inp){
  let tm;
  inp.addEventListener('input',()=>{
    clearTimeout(tm);const q=inp.value.trim();if(q.length<2)return;
    tm=setTimeout(async()=>{try{
      const r=await(await fetch(API+'locations?type=station&fields[]=stations/name&query='+encodeURIComponent(q))).json();
      $('dl').innerHTML=(r.stations||[]).map(s=>s.name).filter(Boolean).slice(0,8).map(n=>'<option value="'+esc(n)+'">').join('');
    }catch(e){}},400);
  });
}

function addSt(st){
  const d=document.createElement('div');d.className='st';
  d.innerHTML='<div class="row"><div class="seg"><button type="button" data-t="stop">Tram &amp; Bus</button><button type="button" data-t="train">Zug</button></div><button type="button" class="sec x">✕</button></div>'
   +'<div class="p-stop"><input class="nm" list="dl" placeholder="Haltestelle" autocomplete="off"><div class="lines"></div></div>'
   +'<div class="p-train"><input class="fr" list="dl" placeholder="Von (z. B. Zürich HB)" autocomplete="off"><input class="to" list="dl" placeholder="Nach (z. B. Winterthur)" autocomplete="off"></div>'
   +'<div class="walk">Fussweg <input class="wk" type="number" min="0" max="60" inputmode="numeric"> min</div>';
  const q=c=>d.querySelector(c);
  const nm=q('.nm'),ln=q('.lines'),fr=q('.fr'),to=q('.to'),wk=q('.wk');
  let type=st.type==='train'?'train':'stop';
  if(type==='train'){fr.value=st.name||'';to.value=st.to||''}else nm.value=st.name||'';
  wk.value=st.walk||0;
  d.sel=new Set((st.lines||'').split(',').filter(Boolean));
  function setType(t){type=t;
    d.querySelectorAll('.seg button').forEach(b=>b.classList.toggle('on',b.dataset.t===t));
    q('.p-stop').style.display=t==='stop'?'':'none';q('.p-train').style.display=t==='train'?'':'none';}
  d.querySelectorAll('.seg button').forEach(b=>b.onclick=()=>setType(b.dataset.t));
  setType(type);
  [nm,fr,to].forEach(suggest);
  d.get=()=>type==='train'
    ?{type,name:fr.value.trim(),to:to.value.trim(),walk:+wk.value||0}
    :{type,name:nm.value.trim(),lines:[...d.sel].join(','),walk:+wk.value||0};
  let last=nm.value.trim();
  nm.onchange=()=>{if(nm.value.trim()!==last){last=nm.value.trim();d.sel.clear();lines()}};
  let seq=0;
  async function lines(){
    const s=nm.value.trim(),my=++seq;if(!s){ln.innerHTML='';return}
    ln.innerHTML='<span class="muted">Linien werden geladen…</span>';
    let l=[];try{
      const r=await(await fetch(API+'stationboard?limit=60&fields[]=stationboard/number&fields[]=stationboard/category&station='+encodeURIComponent(s))).json();
      for(const e of r.stationboard||[])if(e.number&&!l.some(x=>x.n===e.number))l.push({c:e.category||'',n:e.number});
    }catch(e){}
    if(my!==seq)return;
    for(const x of d.sel)if(!l.some(y=>y.n===x))l.push({c:'',n:x});
    if(!l.length){ln.innerHTML='<span class="muted">Nichts gefunden</span>';return}
    l.sort((a,b)=>(parseInt(a.n)||999)-(parseInt(b.n)||999)||a.n.localeCompare(b.n));
    ln.innerHTML='';
    for(const x of l){
      const b=document.createElement('button');b.type='button';
      b.className='chip'+(d.sel.has(x.n)?' on':'');
      b.textContent=(PLAIN.includes(x.c)?'':x.c)+x.n;
      b.onclick=()=>{d.sel.has(x.n)?d.sel.delete(x.n):d.sel.add(x.n);b.classList.toggle('on')};
      ln.appendChild(b);
    }
  }
  q('.x').onclick=()=>{d.remove();upd()};
  $('sts').appendChild(d);upd();if(type==='stop')lines();
}

function colors(){const c={};document.querySelectorAll('#cols input').forEach(i=>c[i.dataset.k]=i.value);return c}
function markPreset(){const c=colors();
  $('pre').querySelectorAll('button').forEach(b=>{const p=PRESETS[b.textContent];b.classList.toggle('on',Object.keys(p).every(k=>c[k]===p[k]))})}
function setCols(c){
  $('cols').innerHTML='';
  for(const k in D.names){
    const l=document.createElement('label');l.textContent=D.names[k];
    const i=document.createElement('input');i.type='color';i.value=c[k]||D.defaults[k];i.dataset.k=k;i.oninput=markPreset;
    $('cols').append(l,i);
  }
  markPreset();
}
for(const n in PRESETS){const b=document.createElement('button');b.type='button';b.className='sec';b.textContent=n;b.onclick=()=>{setCols(PRESETS[n]);if(n==="LED-Tafel")setFont("pixel")};$('pre').appendChild(b)}

// Zweimal tippen zum Bestätigen
function confirmBtn(b,text,url,done){
  let t;const orig=b.textContent;
  b.onclick=async()=>{
    if(!b.classList.contains('warn')){b.classList.add('warn');b.textContent=text;t=setTimeout(()=>{b.classList.remove('warn');b.textContent=orig},4000);return}
    clearTimeout(t);try{await fetch(url,{method:'POST'})}catch(e){}
    document.querySelector('main').innerHTML='<section><p>'+done+'</p></section>';
  };
}
confirmBtn($('wr'),'Nochmal tippen','/api/wifireset','WLAN zurückgesetzt. Scanne den QR-Code auf der Tafel.');
confirmBtn($('fr'),'Alles löschen?','/api/factoryreset','Alles gelöscht. Die Tafel startet mit dem Einrichtungs-Code neu.');

$('non').onchange=()=>$('nt').style.display=$('non').checked?'':'none';

async function load(){
  D=await(await fetch('/api/config')).json();
  const u='http://'+D.ip+'/';$('url').href=u;$('url').textContent=u;
  $('title').value=D.title;$('ver').textContent=D.version?'Version '+D.version:'Version: lokal gebaut';
  $('sts').innerHTML='';D.stations.forEach(addSt);if(!D.stations.length)addSt({});
  $('non').checked=D.night.on;$('nfrom').value=D.night.from;$('nto').value=D.night.to;$('non').onchange();
  setCols(D.colors);setFont(D.font||"sans");
}
$('cp').onclick=async()=>{const t=$('url').href;
  try{await navigator.clipboard.writeText(t)}catch(e){const i=document.createElement('textarea');i.value=t;document.body.append(i);i.select();document.execCommand('copy');i.remove()}
  $('cp').textContent='Kopiert ✓';setTimeout(()=>$('cp').textContent='Kopieren',1500)};
$('add').onclick=()=>addSt({});
$('save').onclick=async()=>{
  const b={title:$('title').value,
    stations:[...$('sts').children].map(d=>d.get()).filter(s=>s.name&&(s.type!=='train'||s.to)),
    night:{on:$('non').checked,from:$('nfrom').value||'00:00',to:$('nto').value||'06:00'},
    font,colors:colors()};
  $('msg').textContent='Speichere…';
  try{
    const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
    $('msg').textContent=r.ok?'Gespeichert ✓':'Fehler beim Speichern';
  }catch(e){$('msg').textContent='Tafel nicht erreichbar'}
  setTimeout(()=>$('msg').textContent='',3000);
};
load().catch(()=>{document.querySelector('main').innerHTML='<section><p>Tafel nicht erreichbar. Ist das Handy im selben WLAN?</p></section>'});
</script></main></body></html>)HTML";
