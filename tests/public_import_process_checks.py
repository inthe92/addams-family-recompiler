"""Exercise frontend -> bundled Python with synthetic files, without a ROM."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def copy_link(source, destination):
    try:
        os.link(source, destination)
    except OSError:
        shutil.copy2(source, destination)
    return destination


def check(probe, resources):
    with tempfile.TemporaryDirectory(prefix='frontend import & (spaces) ') as name:
        root = Path(name)
        package = root/'Application with spaces'
        tools = package/'tools/recompiler'
        shutil.copytree(resources/'tools/recompiler', tools)
        shutil.copytree(resources/'toolchain/python', package/'toolchain/python', copy_function=copy_link)
        profile = json.loads((tools/'compatible_profile.json').read_text())
        fixture = root/'Synthetic input (not a game).nes'
        fixture.write_bytes(b'NES\x1a'+bytes(profile['rom_size']-4))

        def launch(case, expected):
            cache = root/('Cache with spaces '+case)
            # A trailing separator checks Windows backslash-before-quote rules.
            result = subprocess.run([str(probe), '--rom', str(fixture),
                '--import-cache', str(cache)+os.sep, '--import-tools', str(tools)],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
            assert result.returncode != 0, result.stdout
            log = cache/'import.log'
            assert log.is_file(), result.stderr
            message = log.read_text(encoding='utf-8')
            assert expected in message, (message, result.stderr)
            assert not list(cache.glob('user_game.*')), 'Synthetic fixture must not generate game content'

        launch('invalid input', 'Incompatible ROM: SHA-256 mismatch')
        # A module-level error occurs before the importer's own --log handling.
        (tools/'import_user_rom.py').write_text(
            'print("startup stdout", flush=True)\nraise RuntimeError("startup stderr fixture")\n',
            encoding='utf-8')
        launch('early failure', 'startup stderr fixture')
        assert 'startup stdout' in (root/'Cache with spaces early failure/import.log').read_text()
        # Replace only the temporary entry point, leaving hard-linked runtime
        # files intact. This must diagnose failure even when Python never runs.
        executable = package/('toolchain/python/python.exe' if os.name=='nt' else 'toolchain/python/bin/python3')
        executable.unlink()
        executable.write_bytes(b'invalid synthetic executable\n')
        executable.chmod(0o700 if os.name=='nt' else 0o600)
        launch('launch failure', 'Python launch failed')
    print('Frontend importer: spaced paths, early stdout/stderr, launch failure: OK')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('resources', type=Path)
    parser.add_argument('probe', type=Path)
    args = parser.parse_args()
    check(args.probe.resolve(), args.resources.resolve())
