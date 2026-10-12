"""Bind a native frontend and its package to one source revision (no ROM)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIRS = {'.github', 'app', 'assets', 'cmake', 'game', 'include', 'runtime', 'tests', 'tools', 'third_party'}
TEXT_SUFFIXES = {'.c', '.cpp', '.h', '.hpp', '.mm', '.py', '.sh', '.cmake', '.in', '.rc', '.json', '.txt', '.html', '.md', '.yml', '.yaml', '.svg'}


def git(root, *arguments):
    return subprocess.check_output(['git', '-C', str(root), *arguments], stderr=subprocess.DEVNULL)


def source_identity(root=ROOT):
    try:
        commit = git(root, 'rev-parse', 'HEAD').decode().strip()
        dirty = bool(git(root, 'status', '--porcelain', '--untracked-files=normal').strip())
        names = git(root, 'ls-files', '--cached', '--others', '--exclude-standard', '-z').decode().split('\0')
    except (OSError, subprocess.CalledProcessError):
        commit, dirty = None, True
        names = [p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()]
    digest = hashlib.sha256()
    for name in sorted(set(names)):
        if not name:
            continue
        path = Path(name)
        if name != 'CMakeLists.txt' and path.parts[0] not in SOURCE_DIRS:
            continue
        if any(part in {'__pycache__', 'generated', 'user-import', 'build'} for part in path.parts):
            continue
        file = root/path
        if not file.is_file():
            continue
        data = file.read_bytes()
        # Git converts text to CRLF on Windows; binary icon/archive bytes stay exact.
        if file.suffix in TEXT_SUFFIXES or name == 'CMakeLists.txt':
            data = data.replace(b'\r\n', b'\n')
        digest.update(name.encode()+b'\0'+hashlib.sha256(data).digest())
    return commit, dirty, digest.hexdigest()


def current(version=None, system=None, arch=None, root=ROOT):
    cmake = (root/'CMakeLists.txt').read_text()
    configured = re.search(r'project\(nes_native_lab VERSION ([0-9.]+)', cmake).group(1)
    if version is not None and version != configured:
        raise ValueError('Configured version differs from current CMake source')
    abi = int(re.search(r'NES_RECOMP_ABI_VERSION\s+(\d+)u',
                       (root/'include/recomp/module_abi.h').read_text()).group(1))
    commit, dirty, digest = source_identity(root)
    arch = {'AMD64':'x86_64', 'amd64':'x86_64', 'aarch64':'arm64'}.get(arch, arch)
    return dict(schema=1, project='NES Native Lab', version=configured,
                commit=commit, dirty=dirty, source_sha256=digest, abi=abi,
                platform=system, arch=arch)


def write_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def generate(build, version, system, arch, root=ROOT):
    data = current(version, system, arch, root)
    text = json.dumps(data, sort_keys=True)
    write_changed(build/'native-build-info.json', text+'\n')
    write_changed(build/'branding/native_build_metadata.hpp',
                  '// Build identity only; not part of module ABI.\n#pragma once\n'
                  'namespace nes::build { constexpr const char* identity = '+json.dumps(text)+'; }\n')
    return data


def validate(data, expected_commit=None):
    keys = {'schema','project','version','commit','dirty','source_sha256','abi','platform','arch'}
    if set(data) != keys or data['schema'] != 1 or data['project'] != 'NES Native Lab':
        raise ValueError('Invalid package identity schema')
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', data['version']):
        raise ValueError('Invalid package version')
    if data['commit'] is not None and not re.fullmatch(r'[0-9a-f]{40}', data['commit']):
        raise ValueError('Invalid source commit')
    if not re.fullmatch(r'[0-9a-f]{64}', data['source_sha256']) or type(data['dirty']) is not bool:
        raise ValueError('Invalid source fingerprint/state')
    if type(data['abi']) is not int or data['abi'] <= 0:
        raise ValueError('Invalid ABI version')
    if data['platform'] not in {'Darwin','Windows','Linux'} or data['arch'] not in {'arm64','x86_64'}:
        raise ValueError('Invalid target identity')
    if expected_commit and (data['commit'] != expected_commit or data['dirty']):
        raise ValueError('Expected a clean package from exactly '+expected_commit)
    return data


def prepared(build, system, arch):
    data = validate(json.loads((build/'native-build-info.json').read_text()))
    if data != current(system=system, arch=arch):
        raise ValueError('Build metadata is stale: rebuild the frontend from the current source')
    return data


def packaged(resources, frontend, expected_commit=None, run=None):
    info = validate(json.loads((resources/'PACKAGE_INFO.json').read_text()), expected_commit)
    if run is None:
        output = subprocess.check_output([str(frontend), '--build-info'], text=True, timeout=30)
    else:
        output = run([frontend, '--build-info'])
    if json.loads(output) != info:
        raise ValueError('Frontend build identity differs from package metadata')
    header = (resources/'include/recomp/module_abi.h').read_text()
    if info['abi'] != int(re.search(r'NES_RECOMP_ABI_VERSION\s+(\d+)u', header).group(1)):
        raise ValueError('Packaged ABI header differs from compiled frontend')
    return info


def guard_inventory(build):
    inventory = json.loads((build/'ctest-inventory.json').read_text())
    cases = [t for t in inventory['tests'] if t['name'] == 'module_abi_contract']
    if len(cases) != 1 or len(cases[0]['command']) != 5:
        raise ValueError('Missing real native ABI guard inventory')
    return cases[0]['command'][:4]


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, required=True); p.add_argument('--version', required=True)
    p.add_argument('--system', required=True); p.add_argument('--arch', required=True)
    a = p.parse_args(); generate(a.build, a.version, a.system, a.arch)
