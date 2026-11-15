#!/usr/bin/env python3
"""Bundle the local flasher, both development firmwares, sources and licences.

Run after committing: the source snapshot is git HEAD, never untracked local data.
"""
from pathlib import Path
import io
import subprocess
import zipfile

def main():
    root = Path(__file__).resolve().parents[1]
    out = root / 'build' / 'CoralFlower-BT-experimental.zip'
    source = subprocess.check_output(['git', 'archive', '--format=zip', 'HEAD'], cwd=root)
    packages = [root / 'build' / f'coralflower-0.7.1-{kind}dev.fwsc' for kind in ('bt','usb')]
    for path in packages:
        if not path.is_file():
            raise SystemExit(f'Build both variants first: missing {path.name}')
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as archive:
        with zipfile.ZipFile(io.BytesIO(source)) as committed:
            for info in committed.infolist():
                if not info.is_dir():
                    archive.writestr(info.filename, committed.read(info))
        for path in packages:
            archive.write(path, 'firmware-files/' + path.name)
        archive.writestr('START-HERE.txt',
            'Experimental CoralFlower Bluetooth MIDI build. Not hardware validated.\n'
            'Extract this ZIP, then double-click Flash-CoralFlower.cmd (Python 3 required).\n'
            'Use Chrome or Edge. Choose a .fwsc from firmware-files/.\n'
            'btdev has Bluetooth; usbdev is the USB-only fallback.\n'
            'Read docs/BLUETOOTH.md for WIDI Bud Pro setup and limitations.\n'
            'Full corresponding source and licences are included.\n')
    print(out)

if __name__ == '__main__':
    main()
