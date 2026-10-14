"""Export the reviewed cumulative source only; never stage, commit or publish."""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def git(*args):return subprocess.check_output(['git','-C',str(ROOT),*args])
def export(destination,manifest):
 allowlist=ROOT/'docs/MOD_MANAGER_PUBLIC_FILES.txt'
 allowed=allowlist.read_text().splitlines()
 if not allowed or len(allowed)!=len(set(allowed)):raise ValueError('Empty or duplicate public allowlist')
 for name in allowed:
  p=Path(name)
  if p.is_absolute() or '..' in p.parts or name.startswith(('.git/','outputs/','build/')):raise ValueError('Unsafe approved source path')
 unmerged=git('diff','--name-only','--diff-filter=U').decode().strip()
 if unmerged:raise ValueError('Unmerged files require review')
 modified=set(filter(None,git('diff','--name-only','HEAD','-z').decode().split('\0')))
 new=set(filter(None,git('ls-files','--others','--exclude-standard','-z').decode().split('\0')))
 if modified|new!=set(allowed):raise ValueError('Changed file set differs from explicit allowlist: '+', '.join(sorted((modified|new)^set(allowed))))
 if destination.exists() or manifest.exists():raise ValueError('Use a new export directory/manifest; existing files are never replaced')
 names=set(filter(None,git('ls-files','--cached','-z').decode().split('\0')))|set(allowed)
 entries=[]
 for name in sorted(names):
  file=ROOT/name
  if file.is_symlink() or not file.is_file():raise ValueError('Source member missing/linked: '+name)
  if file.suffix.lower() in {'.nes','.rom','.dylib','.dll','.so','.o','.a','.wav','.ppm','.sav','.csv'}:raise ValueError('Private/binary candidate: '+name)
  data=file.read_bytes()
  if re.search(rb'(?:ghp_|github_pat_)[A-Za-z0-9_]{20,}|BEGIN [A-Z ]*PRIVATE KEY',data):raise ValueError('Credential-like content requires review: '+name)
  entries.append(dict(path=name,bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),provenance='new-public' if name in new else 'modified-public' if name in modified else 'historical-base'))
 destination.mkdir(parents=True)
 for entry in entries:
  out=destination/entry['path'];out.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/entry['path'],out)
 manifest.write_text(json.dumps(dict(schema=1,base_commit=git('rev-parse','HEAD').decode().strip(),branch=git('branch','--show-current').decode().strip(),dirty=True,staged=False,changes=sorted(allowed),files=entries),indent=2)+'\n')
 print('Reviewed public source export:',len(entries),'files;',len(allowed),'cumulative changed/new paths; no staging')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--destination',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 try:export(a.destination,a.manifest)
 except (ValueError,OSError,subprocess.CalledProcessError) as e:raise SystemExit('Source export BLOCKED: '+str(e))
