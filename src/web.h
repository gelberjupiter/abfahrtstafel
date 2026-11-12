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
.row input{flex:1;min-width:0}
.note{font-weight:600;margin:12px 0 0;min-height:1.4em}.bad{color:#c00}.good{color:#1a2ab0}
.stw{color:#c00;font-size:14px;font-weight:600;margin:6px 0 0}
#warn{background:#fff4d6;color:#7a5200;font-weight:600}
.ac{position:relative}
.sug{position:absolute;left:0;right:0;top:calc(100% - 6px);z-index:5;background:#fff;border:1px solid #ccd;border-radius:10px;box-shadow:0 8px 24px #0003;overflow:hidden}
.sug button{display:flex;gap:10px;align-items:center;width:100%;margin:0;padding:11px 12px;background:#fff;color:#1b1f3a;font-weight:500;border-radius:0;text-align:left;border-top:1px solid #f0f1f6}
.sug button:first-child{border-top:0}.sug button:active{background:#eef0fa}
.sug small{margin-left:auto;color:#889;font-weight:400;font-size:13px}
.hint{color:#778;font-size:14px;margin:0 0 8px}
.lg{margin-top:10px}.lg b{display:block;font-size:13px;color:#667;margin:0 0 6px;font-weight:600}.lg .lines{margin-top:0}
.spin{display:inline-block;width:14px;height:14px;border:2px solid #1a2ab0;border-right-color:transparent;border-radius:50%;animation:r .8s linear infinite;vertical-align:-2px;margin-right:6px}@keyframes r{to{transform:rotate(1turn)}}
</style></head><body><header><h1>Abfahrtstafel</h1></header><main>)HTML";

// ---------- Einrichtung (eigenes WLAN der Tafel, öffnet sich am Handy von selbst) ----------
const char PORTAL_HTML[] PROGMEM = R"HTML(
<section id="f"><h2>Mit deinem WLAN verbinden</h2>
<label for="s">WLAN</label><select id="s"><option>Suche…</option></select>
<input id="o" placeholder="Name des WLANs" autocapitalize="off" autocorrect="off" spellcheck="false" style="display:none;margin-top:8px">
<button type="button" class="sec" id="r">Neu suchen</button>
<label for="p">Passwort</label>
<div class="row"><input id="p" type="password" autocomplete="off" autocapitalize="off" autocorrect="off" spellcheck="false"><button type="button" class="sec" id="e">Zeigen</button></div>
<button type="button" id="go">Verbinden</button>
<p id="m" class="note"></p></section>
<p class="muted" id="hint">Haltestellen und Farben stellst du danach über den Code auf der Tafel ein.</p>
<script>
const $=i=>document.getElementById(i),s=$('s'),o=$('o'),p=$('p'),m=$('m'),go=$('go');
const esc=t=>String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const sleep=t=>new Promise(r=>setTimeout(r,t));
function say(t,cls){m.innerHTML=t;m.className='note '+(cls||'bad')}
async function load(re){
  s.innerHTML='<option>Suche…</option>';
  let l=[];try{l=await(await fetch('/api/networks'+(re?'?rescan=1':''))).json()}catch(e){}
  s.innerHTML='';
  for(const n of l)s.add(new Option(n.ssid+(n.lock?' 🔒':''),n.ssid));
  s.add(new Option('Anderes WLAN…','__other'));
  s.onchange();
}
s.onchange=()=>o.style.display=s.value=='__other'?'block':'none';
$('r').onclick=()=>load(1);
$('e').onclick=()=>{const show=p.type=='password';p.type=show?'text':'password';$('e').textContent=show?'Verbergen':'Zeigen'};
p.onkeydown=e=>{if(e.key=='Enter')go.click()};
go.onclick=async()=>{
  const ssid=s.value=='__other'?o.value.trim():s.value;
  if(!ssid||ssid=='Suche…'){say('Bitte zuerst dein WLAN auswählen.');return}
  go.disabled=true;say('<span class="spin"></span>Verbinde mit «'+esc(ssid)+'»…','good');
  try{const r=await fetch('/api/connect',{method:'POST',body:new URLSearchParams({ssid,pass:p.value})});if(!r.ok)throw 0}
  catch(e){say('Tafel antwortet nicht. Ist das Handy noch im WLAN «Abfahrtstafel»?');go.disabled=false;return}
  for(let miss=0;;){
    await sleep(1000);
    let st;try{st=(await(await fetch('/api/state',{cache:'no-store'})).json()).s;miss=0}
    catch(e){if(++miss>25){say('Verbindung zur Tafel unterbrochen. Schau auf die Tafel: Steht dort «Verbunden», hat alles geklappt.');break}continue}
    if(st==1)continue;
    if(st==2){$('f').innerHTML='<h2>Verbunden ✓</h2><p>Die Tafel ist jetzt mit «'+esc(ssid)+'» verbunden und startet neu.</p><p class="muted">Dieses Fenster schliesst sich gleich von selbst. Danach den Code auf der Tafel scannen, um Haltestellen einzustellen.</p>';$('hint').hidden=true;return}
    if(st==3){say('Passwort falsch. Bitte nochmals eingeben, Gross- und Kleinschreibung beachten.');p.focus();p.select()}
    else if(st==4)say('«'+esc(ssid)+'» nicht gefunden. Die Tafel braucht ein 2,4-GHz-WLAN in der Nähe.');
    else say('Verbindung hat nicht geklappt. Bitte nochmals versuchen.');
    break;
  }
  go.disabled=false;
};
load(0);
</script></main></body></html>)HTML";

// ---------- Einstellungen (im Heim-WLAN) ----------
const char CONFIG_HTML[] PROGMEM = R"HTML(
<section id="warn" hidden></section>
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
<label>Schrift</label><div class="seg" id="font"><button type="button" data-f="sans">Normal</button><button type="button" data-f="pixel">Pixel (LED)</button><button type="button" data-f="flap">Fallblatt</button></div>
<label>Farben</label><div class="presets" id="pre"></div>
<details><summary>Eigene Farben</summary><div id="cols" class="grid" style="margin-top:10px"></div></details></section>

<div class="bar"><button id="save">Speichern</button><p id="msg"></p></div>

<section><h2>Gerät</h2><div class="two">
<button type="button" class="sec" id="wr">WLAN ändern</button>
<button type="button" class="sec danger" id="fr">Werkseinstellungen</button></div>
<p class="muted" id="ver" style="text-align:center;margin-top:10px"></p></section>
<script>
const $=i=>document.getElementById(i);
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const SCH="https://search.ch/timetable/api/",OD="https://transport.opendata.ch/v1/",enc=encodeURIComponent;
async function getJ(u,ms){const c=new AbortController(),t=setTimeout(()=>c.abort(),ms||7000);
  try{const r=await fetch(u,{signal:c.signal,cache:'no-store'});if(!r.ok)throw r.status;return await r.json()}finally{clearTimeout(t)}}
// Fahrplan-Daten: zuerst search.ch, sonst transport.opendata.ch
const KIND={train:['🚆','Zug'],tram:['🚊','Tram'],bus:['🚌','Bus'],ship:['⛴️','Schiff'],other:['🚡','Bahn']};
const GROUP={train:'Züge',tram:'Tram',bus:'Bus',ship:'Schiff',other:'Weitere'};
const typeKind=t=>/train/.test(t)?'train':/tram/.test(t)?'tram':/bus|post/.test(t)?'bus':/ship|boat/.test(t)?'ship':/cable|funi|gondola|chair/.test(t)?'other':'';
const catKind=c=>/^(T|NFT|TRAM)$/.test(c)?'tram':/^(B|NFB|BUS|KB)$/.test(c)?'bus':/^(BAT|FAE)$/.test(c)?'ship':'train';
async function schNames(q){
  return(await getJ(SCH+'completion.json?term='+enc(q))).filter(x=>/sl-icon-type-/.test(x.iconclass||'')&&!/adr|poi/.test(x.iconclass))
    .map(x=>({n:x.label.trim(),k:typeKind(x.iconclass)}))}
async function findNames(q){
  // "Winterthur HB" findet bei search.ch nur Bushaltestellen: Bahnhof "Winterthur" zuerst vorschlagen
  const m=q.match(/^(.+?)[\s,]+(hb|hbf|hauptbahnhof|bahnhof|bhf|sbb)\.?$/i);
  try{
    const[a,b]=await Promise.all([schNames(q),m?schNames(m[1]).catch(()=>[]):[]]);
    const l=[...b.filter(x=>x.k==='train').slice(0,1),...a];
    return l.filter((x,i)=>l.findIndex(y=>y.n===x.n)===i);
  }catch(e){}
  try{return((await getJ(OD+'locations?type=station&fields[]=stations/name&query='+enc(q))).stations||[]).map(s=>({n:s.name,k:''}))}catch(e){}
  return[]}
async function findLines(s){
  try{const r=await getJ(SCH+'stationboard.json?limit=80&stop='+enc(s));
    if(r.connections)return r.connections.map(e=>{const c=e['*G']||'';return{c,n:e['*L']||e.line||'',k:typeKind(e.type||'')||catKind(c)}});if(r.messages)return null}catch(e){}
  try{const r=await getJ(OD+'stationboard?limit=80&fields[]=station/name&fields[]=stationboard/number&fields[]=stationboard/category&station='+enc(s));
    if(r.station&&r.station.name)return(r.stationboard||[]).map(e=>({c:e.category||'',n:e.number||'',k:catKind(e.category||'')}));return null}catch(e){}
  return false}
const PLAIN=['T','B','NFT','NFB','TRAM','BUS',''];
const PRESETS={
  'SBB-Blau':{bg:'#1a2ab0',hdr:'#101878',line:'#4055d8',text:'#ffffff',delay:'#ffde15',muted:'#b4b8e6',go:'#ff4a3d'},
  'LED-Tafel':{bg:'#000000',hdr:'#000000',line:'#2a2a2a',text:'#ffb000',delay:'#ff5a00',muted:'#8a6a20',go:'#ff2a2a'},
  'Dunkel':{bg:'#15171c',hdr:'#0b0c0f',line:'#2c303a',text:'#e8e8ea',delay:'#ffcc33',muted:'#8b90a0',go:'#ff5a4d'}};
let D,font="sans";
function setFont(f){font=f;document.querySelectorAll("#font button").forEach(b=>b.classList.toggle("on",b.dataset.f===f))}
document.querySelectorAll("#font button").forEach(b=>b.onclick=()=>setFont(b.dataset.f));

function upd(){$('add').disabled=$('sts').children.length>=4}

// Vorschläge beim Tippen (Handy fragt den Fahrplan-Server direkt), mit Verkehrsmittel-Symbol
function suggest(inp){
  const box=document.createElement('div');box.className='sug';box.hidden=true;inp.after(box);
  let tm,seq=0;const close=()=>{box.hidden=true};
  inp.addEventListener('input',()=>{
    clearTimeout(tm);const q=inp.value.trim();if(q.length<2){close();return}
    tm=setTimeout(async()=>{
      const my=++seq,l=(await findNames(q)).slice(0,7);
      if(my!==seq||document.activeElement!==inp)return;
      box.innerHTML='';
      for(const x of l){
        const b=document.createElement('button');b.type='button';
        b.innerHTML='<span>'+(KIND[x.k]||['📍'])[0]+'</span><span>'+esc(x.n)+'</span>'+(x.k?'<small>'+KIND[x.k][1]+'</small>':'');
        b.onpointerdown=e=>e.preventDefault();
        b.onclick=()=>{inp.value=x.n;close();inp.dispatchEvent(new Event('change'));inp.blur()};
        box.appendChild(b);
      }
      box.hidden=!l.length;
    },300);
  });
  inp.addEventListener('blur',()=>setTimeout(close,200));
}

function addSt(st){
  const d=document.createElement('div');d.className='st';
  d.innerHTML='<div class="row"><div class="seg"><button type="button" data-t="stop">Station</button><button type="button" data-t="train">Verbindung</button></div><button type="button" class="sec x">✕</button></div>'
   +'<div class="p-stop"><p class="hint">Alle Abfahrten ab einem Bahnhof oder einer Haltestelle: Züge, Trams und Busse.</p><div class="ac"><input class="nm" placeholder="z. B. Zürich HB" autocomplete="off" autocorrect="off" spellcheck="false"></div><div class="lns"></div></div>'
   +'<div class="p-train"><p class="hint">Nur direkte Verbindungen von A nach B, ohne Umsteigen.</p><div class="ac"><input class="fr" placeholder="Von, z. B. Zürich HB" autocomplete="off" autocorrect="off" spellcheck="false"></div><div class="ac"><input class="to" placeholder="Nach, z. B. Winterthur" autocomplete="off" autocorrect="off" spellcheck="false"></div></div>'
   +'<div class="walk">Fussweg <input class="wk" type="number" min="0" max="60" inputmode="numeric"> min</div><p class="stw" hidden></p>';
  const q=c=>d.querySelector(c);
  const nm=q('.nm'),ln=q('.lns'),fr=q('.fr'),to=q('.to'),wk=q('.wk'),w=q('.stw');
  d.warn=t=>{w.textContent=t?'⚠ '+t:'';w.hidden=!t};
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
  [nm,fr,to].forEach(i=>i.addEventListener('input',()=>d.warn('')));
  let last=nm.value.trim();
  nm.onchange=()=>{if(nm.value.trim()!==last){last=nm.value.trim();d.sel.clear();lines()}};
  let seq=0;
  async function lines(){
    const s=nm.value.trim(),my=++seq;if(!s){ln.innerHTML='';return}
    ln.innerHTML='<span class="muted">Linien werden geladen…</span>';
    const r=await findLines(s);
    if(my!==seq)return;
    const l=[];
    for(const e of r||[])if(e.n&&!l.some(x=>x.n===e.n))l.push(e);
    for(const x of d.sel)if(!l.some(y=>y.n===x))l.push({c:'',n:x,k:'other'});
    if(r===null){ln.innerHTML='';d.warn('Nicht gefunden. Bitte einen Namen aus der Vorschlagsliste wählen.');return}
    if(!l.length){ln.innerHTML='<p class="muted">'+(r===false?'Linien können gerade nicht geladen werden. Speichern geht trotzdem (zeigt alle Linien).':'Gerade keine Abfahrten gefunden.')+'</p>';return}
    l.sort((a,b)=>(parseInt(a.n)||999)-(parseInt(b.n)||999)||a.n.localeCompare(b.n));
    ln.innerHTML='<p class="muted" style="margin-top:10px">Nur bestimmte Linien zeigen? Antippen, sonst kommen alle.</p>';
    for(const k of['train','tram','bus','ship','other']){
      const g=l.filter(x=>(x.k||'other')===k);if(!g.length)continue;
      const box=document.createElement('div');box.className='lg';
      box.innerHTML='<b>'+KIND[k][0]+' '+GROUP[k]+'</b><div class="lines"></div>';
      for(const x of g){
        const b=document.createElement('button');b.type='button';
        b.className='chip'+(d.sel.has(x.n)?' on':'');
        b.textContent=PLAIN.includes(x.c)||x.n.startsWith(x.c)?x.n:x.c+(x.c.length>1?' ':'')+x.n;
        b.onclick=()=>{d.sel.has(x.n)?d.sel.delete(x.n):d.sel.add(x.n);b.classList.toggle('on')};
        box.lastChild.appendChild(b);
      }
      ln.appendChild(box);
    }
    // Bushaltestelle "…/HB" gewählt, aber Züge gemeint? Bahnhof anbieten
    const m=!l.some(x=>x.k==='train')&&s.match(/^([^,]+),.*\b(HB|Hbf|Hauptbahnhof|Bahnhof|Bhf)\b/i);
    if(m){
      const t=(await schNames(m[1]).catch(()=>[])).find(x=>x.k==='train');
      if(t&&my===seq){
        const b=document.createElement('button');b.type='button';b.className='sec';
        b.textContent='🚆 Züge gesucht? Bahnhof «'+t.n+'» nehmen';
        b.onclick=()=>{nm.value=t.n;nm.onchange()};
        ln.prepend(b);
      }
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

// Zustand der Tafel: Warnung oben und pro Eintrag
let SAVED=[];
function showStatus(S){
  if(!S)return;
  SAVED.forEach((d,i)=>d.warn(S.st[i]==2?'Nicht gefunden. Bitte einen Namen aus der Vorschlagsliste wählen.':''));
  const w=$('warn');
  w.textContent=!S.wifi?'⚠ Die Tafel hat gerade kein WLAN und verbindet sich neu.'
    :S.st.length&&S.st.every(x=>x==3)?'⚠ Der Fahrplan-Server ist gerade nicht erreichbar. Die Tafel versucht es automatisch weiter.':'';
  w.hidden=!w.textContent;
}
async function poll(){
  for(let i=0;i<15;i++){
    await new Promise(r=>setTimeout(r,1500));
    let S;try{S=await getJ('/api/status',4000)}catch(e){continue}
    if(!S.st.includes(0)||i==14){showStatus(S);return S}
  }
}
setInterval(async()=>{try{showStatus(await getJ('/api/status',4000))}catch(e){}},30000);

async function load(){
  D=await getJ('/api/config',8000);
  const u='http://'+D.host+'/';$('url').href=u;$('url').textContent=u;
  $('title').value=D.title;$('ver').textContent=D.version?'Version '+D.version:'Version: lokal gebaut';
  $('sts').innerHTML='';D.stations.forEach(addSt);if(!D.stations.length)addSt({});
  SAVED=[...$('sts').children];showStatus(D.status);
  $('non').checked=D.night.on;$('nfrom').value=D.night.from;$('nto').value=D.night.to;$('non').onchange();
  setCols(D.colors);setFont(D.font||"sans");
}
$('cp').onclick=async()=>{const t=$('url').href;
  try{await navigator.clipboard.writeText(t)}catch(e){const i=document.createElement('textarea');i.value=t;document.body.append(i);i.select();document.execCommand('copy');i.remove()}
  $('cp').textContent='Kopiert ✓';setTimeout(()=>$('cp').textContent='Kopieren',1500)};
$('add').onclick=()=>addSt({});
let msgT;
function msg(t,keep){$('msg').textContent=t;clearTimeout(msgT);if(!keep)msgT=setTimeout(()=>$('msg').textContent='',4000)}
$('save').onclick=async()=>{
  const els=[],stations=[];let bad=false;
  for(const d of $('sts').children){const s=d.get();d.warn('');
    if(s.type==='train'&&(s.name||s.to)&&!(s.name&&s.to)){d.warn(s.name?'Bitte ein Ziel eingeben.':'Bitte einen Start eingeben.');bad=true}
    else if(s.name){els.push(d);stations.push(s)}}
  if(bad){msg('Bitte die markierten Felder ergänzen.');return}
  const b={title:$('title').value,stations,
    night:{on:$('non').checked,from:$('nfrom').value||'00:00',to:$('nto').value||'06:00'},
    font,colors:colors()};
  $('save').disabled=true;msg('Speichere…',1);
  try{
    const c=new AbortController(),t=setTimeout(()=>c.abort(),8000);
    const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b),signal:c.signal});
    clearTimeout(t);if(!r.ok)throw 0;
    SAVED=els;msg('Gespeichert ✓ Prüfe Haltestellen…',1);
    const S=await poll();
    msg(!S?'Gespeichert ✓':S.st.includes(2)?'Gespeichert, aber eine Haltestelle wurde nicht gefunden.':S.st.length&&S.st.every(x=>x==3)?'Gespeichert ✓ Fahrplan gerade nicht erreichbar.':'Gespeichert ✓ Tafel ist aktuell.');
  }catch(e){msg('Speichern fehlgeschlagen. Ist das Handy im selben WLAN wie die Tafel?')}
  $('save').disabled=false;
};
load().catch(()=>{document.querySelector('main').innerHTML='<section><h2>Tafel nicht erreichbar</h2><p>Ist das Handy im selben WLAN wie die Tafel? Dann Seite neu laden.</p><button onclick="location.reload()">Neu laden</button></section>'});
</script></main></body></html>)HTML";
