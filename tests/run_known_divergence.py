"""Legacy filename retained; diagnostic failures now always fail, never skip."""
import subprocess
import sys

def outcome(result):
    print(result.stdout, end='')
    print(result.stderr, end='', file=sys.stderr)
    if result.returncode == 0:
        return 0
    print('Diagnostic failure: no expected-divergence exemption remains.', file=sys.stderr)
    return 1


if __name__ == '__main__':
    result = subprocess.run([sys.argv[1]], capture_output=True, text=True, timeout=20)
    sys.exit(outcome(result))
