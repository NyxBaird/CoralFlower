// SPDX-License-Identifier: GPL-3.0-only
import { MAX_PACKAGE, validatePackage, SingleDeviceUpdater, inspect, flash, friendlyError } from './core.js';
import { backupFileName } from '../fm1backup.js';
const $ = id => document.getElementById(id);
let pkg = null, busy = false, epoch = 0, access, writing = false;
const lines = [];
function log(text) {
  lines.push(`${new Date().toISOString()} ${text}`);
  if (lines.length > 1500) lines.shift();
  $('log').textContent = lines.join('\n');
}
function lock(on) {
  busy = on;
  for (const id of ['file','ack']) $(id).disabled = on;
  $('connect').disabled = on || !navigator.requestMIDIAccess || !isSecureContext;
  $('install').disabled = on || !pkg || !navigator.requestMIDIAccess || !isSecureContext;
}
function download(content, name, type) {
  const url = URL.createObjectURL(new Blob([content], {type}));
  const a = document.createElement('a');
  a.href = url; a.download = name; document.body.append(a); a.click(); a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 60000);
}
async function choose(file) {
  if (busy || !file) return;
  const ticket = ++epoch;
  pkg = null; lock(true); $('hash').textContent = '';
  $('package').textContent = `Checking ${file.name}…`;
  try {
    if (file.size > MAX_PACKAGE) throw new Error('File too large. Choose an FM-1 .fwsc file, at most 4 MB.');
    const checked = await validatePackage(new Uint8Array(await file.arrayBuffer()));
    if (ticket !== epoch) return;
    pkg = checked;
    $('package').textContent = `${file.name} · ${checked.stock ? 'Original M-VAVE V15' : 'CoralFlower'} · checks passed`;
    $('hash').textContent = `SHA-256 ${pkg.sha}`;
    $('install').firstChild.textContent = checked.stock ? 'Install official V15 ' : 'Install CoralFlower ';
    log(`Package ${file.name}; ${file.size} bytes; ${pkg.product}; SHA-256 ${pkg.sha}`);
  } catch (e) { $('package').textContent = friendlyError(e); log(`File refused: ${e.message}`); }
  finally { if (ticket === epoch) lock(false); }
}
async function updater() {
  if (!navigator.requestMIDIAccess || !isSecureContext) throw new Error('Use Chrome or Edge and open this tool on localhost or HTTPS.');
  access ||= await navigator.requestMIDIAccess({sysex:true});
  return new SingleDeviceUpdater(access, log);
}
$('file').addEventListener('change', () => choose($('file').files[0]));
for (const event of ['dragover','dragleave','drop']) $('drop').addEventListener(event, e => {
  e.preventDefault(); $('drop').classList.toggle('over', event === 'dragover' && !busy);
  if (event === 'drop' && !busy) choose(e.dataTransfer.files[0]);
});
$('connect').addEventListener('click', async () => {
  if (busy) return; lock(true); $('device').textContent = 'Looking for your FM-1…';
  try {
    const unit = await inspect(await updater());
    $('device').textContent = `${unit.name} · ${unit.identity}${unit.verdict === 'refuse' ? ' · installation blocked' : ''}`;
    log(`Device: ${unit.identity}; ${unit.name}; ${unit.verdict}`);
  } catch(e) { $('device').textContent = friendlyError(e); log(e.message); }
  finally { lock(false); }
});
const stage = {identify:0, backup:1, start:2, verify:2, loader:2, resume:3, write:3, reboot:4, done:5};
const words = {identify:'Identifying your FM-1…', backup:'Saving your sounds…', start:'Starting the update…', verify:'Checking firmware on the FM-1…', loader:'Waiting for update mode…', resume:'Resuming the update…', write:'Writing firmware. Keep USB connected.', reboot:'Waiting for the FM-1 to restart…', done:'Installed. Ready to play.'};
function progress(key, data) {
  $('state').textContent = words[key] || key;
  [...$('steps').children].forEach((el,i) => el.classList.toggle('active', i <= stage[key]));
  if (key === 'backup' && data) { $('progress').value = data.done / data.total * 100; $('detail').textContent = `${data.done.toLocaleString()} / ${data.total.toLocaleString()} backup bytes`; }
  else if (key === 'done') { $('progress').value = 100; $('detail').textContent = 'The FM-1 returned with the expected firmware identity. You can disconnect USB.'; }
  else { $('progress').removeAttribute('value'); $('detail').textContent = typeof data === 'number' ? `${data} blocks served in this stage.` : 'Keep this page open and the USB cable connected.'; }
  if (['start','resume','write'].includes(key)) writing = true;
  if (!['write','verify','backup'].includes(key) || typeof data !== 'number' || data % 50 === 0)
    log(`${key}${data === undefined ? '' : ' ' + JSON.stringify(data)}`);
}
async function saveBackup(archive) {
  const name = backupFileName(archive.firmware);
  download(JSON.stringify(archive,null,2), name, 'application/json');
  log(`Backup download requested: ${name}`);
  $('state').textContent = 'Keep your backup before continuing.';
  $('detail').textContent = `Check that ${name} is saved in Downloads. Nothing has been flashed yet.`;
  await new Promise((resolve,reject) => {
    const row = document.createElement('div');
    const next = document.createElement('button'), cancel = document.createElement('button');
    next.textContent = 'Backup saved — continue'; cancel.textContent = 'Cancel'; cancel.className = 'text';
    next.onclick = () => { row.remove(); resolve(); };
    cancel.onclick = () => { row.remove(); reject(new Error('Cancelled before flashing. Your backup is kept.')); };
    row.append(next,cancel); $('detail').after(row); next.focus();
  });
}
$('install').addEventListener('click', async () => {
  if (busy || !pkg) return;
  lock(true); writing = false; $('recovery').hidden = true;
  const started = Date.now();
  const timer = setInterval(() => { $('elapsed').textContent = `${Math.floor((Date.now()-started)/1000)}s elapsed`; }, 1000);
  let wake;
  try {
    try { wake = await navigator.wakeLock?.request('screen'); } catch (_) { /* optional */ }
    await flash({up:await updater(),pkg,allowNoBackup:$('ack').checked,saveBackup,progress});
    progress('done');
  } catch(e) {
    $('state').textContent = writing ? 'Update needs attention.' : 'Stopped before flashing.';
    $('detail').textContent = friendlyError(e); $('recovery').hidden = !writing;
    $('progress').value = 0; log(`ERROR ${e.code || e.name}: ${e.message}`);
  } finally { clearInterval(timer); await wake?.release().catch(()=>{}); writing = false; lock(false); }
});
$('save-log').onclick = () => download(lines.join('\n'), 'coralflower-flash-log.txt', 'text/plain');
window.addEventListener('beforeunload', e => { if (busy) { e.preventDefault(); e.returnValue = ''; } });
if (!navigator.requestMIDIAccess || !isSecureContext) {
  $('state').textContent = 'Open in Chrome or Edge.';
  $('detail').textContent = 'Use the supplied launcher to open localhost, or serve this folder over HTTPS. Web MIDI with SysEx is required.';
  $('connect').disabled = true;
}
log('Flasher ready. No writes performed.');
