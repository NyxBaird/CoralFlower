// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { validatePackage, flash, SingleDeviceUpdater, friendlyError } from './core.js';
import { bkU32, bkPack, bkCrc } from '../fm1backup.js';

const pkg = {product:'FM-1_930',image:new Uint8Array(2048)};
function fake({identity='FM-1_930',version='CoralFlower 0.7.1',broken=false,resumeOK=true}={}) {
  const events=[], data=new Uint8Array([1,2,3,4]);
  const input={state:'connected',onmidimessage:null};
  const output={state:'connected',send(packet) {
    const cmd=packet[4]; events.push(`cmd:${cmd}`);
    let payload;
    if(cmd===1) payload=[...Buffer.from(version),0];
    else if(cmd===65) payload=broken ? [1,4,0] : [1,0,1,6,...bkU32(data.length),...bkU32(bkCrc(data))];
    else if(cmd===66) payload=[6,0,...bkU32(0),data.length,0,...bkPack(data)];
    queueMicrotask(()=>input.onmidimessage?.({data:[240,125,70,76,cmd,...payload,247]}));
  }};
  const id={text:identity,model:identity.startsWith('ota-')?'ota-FM-1':'FM-1'};
  return {events,async find(){return {id,link:{input,output,close(){input.onmidimessage=null;}}};},
    async install(image,product){events.push(`install:${product}`);},
    async resume(image,progress,product){events.push(`resume:${product}`);return resumeOK;}};
}
{
  const up=fake();
  await flash({up,pkg,saveBackup:async archive=>{assert.equal(archive.objects[0].data,'AQIDBA==');up.events.push('saved');}});
  assert.ok(up.events.indexOf('saved') < up.events.indexOf('install:FM-1_930'));
}
for (const mode of ['broken','cancel']) {
  const up=fake({broken:mode==='broken'});
  await assert.rejects(flash({up,pkg,saveBackup:async()=>{throw new Error('cancelled');}}));
  assert.ok(!up.events.some(e=>e.startsWith('install:')));
}
{
  const up=fake({identity:'FM-1_015',version:''});
  await assert.rejects(flash({up,pkg}),/cannot back up/);
  assert.ok(!up.events.some(e=>e.startsWith('install:')));
  await flash({up,pkg,allowNoBackup:true});
  assert.ok(up.events.includes('install:FM-1_930'));
}
{
  const up=fake({identity:'FM-1_920',version:'Sloop 2.2'});
  await assert.rejects(flash({up,pkg,allowNoBackup:true}),/not supported/);
  assert.ok(!up.events.some(e=>e.startsWith('install:')));
}
for(const resumeOK of [true,false]) {
  const up=fake({identity:'ota-FM-1_930',resumeOK});
  const task=flash({up,pkg});
  if(resumeOK) await task; else await assert.rejects(task,/did not finish/);
  assert.deepEqual(up.events,['resume:FM-1_930']);
}
{
  const access={inputs:new Map([['a',{name:'FM-1',state:'connected'}],['b',{name:'FM-1',state:'connected'}]]),outputs:new Map()};
  await assert.rejects(new SingleDeviceUpdater(access).find(),/More than one/);
}
await assert.rejects(validatePackage(new Uint8Array(10)),/Choose an FM-1/);
await assert.rejects(validatePackage(new Uint8Array((4<<20)+1)),/maximum/);
assert.match(friendlyError({code:'lost'}),/reconnect/i);
if (process.argv[2]) {
  const bytes = new Uint8Array(readFileSync(process.argv[2]));
  const checked = await validatePackage(bytes);
  assert.equal(checked.product,'FM-1_930'); assert.equal(checked.sha.length,64);
  const damaged = bytes.slice(); damaged[2000]^=1;
  await assert.rejects(validatePackage(damaged));
  console.log('Real firmware accepted; damaged firmware rejected.');
}
console.log('Flasher checks passed: backup before write, failed/cancelled backup, explicit no-backup, firmware refusal, resume, multiple devices, package validation.');
