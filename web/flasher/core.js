// SPDX-License-Identifier: GPL-3.0-only
import { Package, productOf, logicalImage, crc16, sha256hex, validateStockPackage, STOCK_APP_SIZE } from '../fm1pkg.js';
import { Updater, classifyFirmware, refusalText } from '../fm1ota.js';
import { BackupConnection, deviceInfo, captureBackup } from '../fm1backup.js';

export const MAX_PACKAGE = 4 << 20;
export async function validatePackage(bytes) {
  if (bytes.length < 1984 || bytes.length > MAX_PACKAGE) throw new Error('Choose an FM-1 .fwsc package (maximum 4 MB).');
  const product = productOf(bytes);
  if (product === 'FM-1_015') {
    await validateStockPackage(bytes); // only the known, unmodified V15 checksum
    return { product, image: logicalImage(bytes), sha: await sha256hex(bytes), stock: true };
  }
  if (product !== 'FM-1_930') throw new Error(`Unsupported package identity: ${product || 'unknown'}. Use CoralFlower or official V15.`);
  const pkg = new Package(bytes);
  const ota = pkg.entries.find(e => e.type === 100 && e.name === 'ota.bin');
  if (!ota || pkg.flashSize !== 0x93000 || pkg.appBase !== 0x4000 || pkg.app.size !== STOCK_APP_SIZE)
    throw new Error('Unexpected flash layout. This package cannot be installed.');
  const start = ota.dataOff + pkg.skew;
  if (start + ota.size > bytes.length) throw new Error('The update loader is truncated.');
  const loader = bytes.subarray(start, start + ota.size);
  if (crc16(loader) !== ota.dcrc || !new TextDecoder('latin1').decode(loader).includes('FELUCCA-LOADER-1'))
    throw new Error('Missing or damaged recovery-compatible update loader.');
  return { product, image: logicalImage(bytes), sha: await sha256hex(bytes), stock: false };
}

// Re-evaluate at every handshake, including re-enumeration. Never silently pick
// one of several instruments or cross-pair identically named ports.
export class SingleDeviceUpdater extends Updater {
  async find(filter) {
    const candidate = p => p.state !== 'disconnected' && /fm-1|felucca|coralflower|ota|composite|sinco|usb-midi/i.test(p.name || '');
    const ins = [...this.access.inputs.values()].filter(candidate);
    const outs = [...this.access.outputs.values()].filter(candidate);
    if (ins.length > 1 || outs.length > 1)
      throw new Error('More than one possible FM-1 MIDI port is connected. Disconnect the other instruments, then retry.');
    return super.find(filter);
  }
}

export async function inspect(up) {
  const dev = await up.find();
  if (!dev) throw new Error('FM-1 not found. Use a USB data cable and close your DAW or other MIDI apps.');
  const { input, output } = dev.link;
  dev.link.close();
  if (!['FM-1', 'ota-FM-1'].includes(dev.id.model)) throw new Error('This device is not an FM-1.');
  if (dev.id.model === 'ota-FM-1') return { identity: dev.id.text, verdict: 'loader', name: 'FM-1 in update mode' };
  const link = new BackupConnection(input, output);
  try {
    const info = await deviceInfo((r,o) => link.request(r,o));
    return { ...classifyFirmware(dev.id.text, info?.version), identity: dev.id.text, version: info?.version };
  } finally { link.close(); }
}

export async function backup(up, progress) {
  const dev = await up.find(id => id.model === 'FM-1');
  if (!dev) throw new Error('The FM-1 disconnected before backup. Nothing was written.');
  const { input, output } = dev.link;
  dev.link.close();
  const link = new BackupConnection(input, output);
  try {
    const request = (r,o) => link.request(r,o);
    const info = await deviceInfo(request);
    if (!info) throw new Error('The firmware did not answer the backup request. Nothing was written.');
    return await captureBackup(request, info.version, progress);
  } finally { link.close(); }
}

export async function flash({ up, pkg, allowNoBackup, saveBackup, progress = () => {} }) {
  progress('identify');
  const unit = await inspect(up); // fresh identity, never rely on an earlier check
  if (unit.verdict === 'refuse') throw new Error(refusalText(unit.name));
  if (unit.verdict === 'loader') {
    progress('resume');
    if (!await up.resume(pkg.image, progress, pkg.product))
      throw new Error('The update did not finish. Keep the same package, reconnect USB, then retry to resume.');
    return;
  }
  if (unit.version) {
    progress('backup');
    const archive = await backup(up, (done,total) => progress('backup', {done,total}));
    // Must resolve only after a backup has been saved; failure stops the write.
    await saveBackup(archive);
  } else if (!allowNoBackup) {
    throw new Error('This firmware cannot back up its sounds. Tick the acknowledgement below before installing.');
  }
  const current = await inspect(up);
  if (current.identity !== unit.identity || current.version !== unit.version || current.verdict !== unit.verdict)
    throw new Error('The connected firmware changed during backup. Nothing was written; check the connection and retry.');
  await up.install(pkg.image, pkg.product, progress);
}

export function friendlyError(error) {
  const fixes = {
    NotAllowedError: 'MIDI access was denied. Allow MIDI with SysEx in your browser’s site permissions, then reconnect.',
    SecurityError: 'Open this page on localhost or HTTPS in Chrome or Edge.',
    noloader: 'The update loader did not appear. Reconnect USB and retry with the same file.',
    lost: 'USB disconnected. Reconnect and retry with the same file to resume the update.',
    noreturn: 'The write finished, but restart could not be verified. Power-cycle the FM-1 and use Check connection.',
    mismatch: 'The device restarted with an unexpected identity. Save the log before doing anything else.'
  };
  return fixes[error.code] || fixes[error.name] || error.message || String(error);
}
