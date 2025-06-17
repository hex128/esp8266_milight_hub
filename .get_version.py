import os
import sys
import subprocess

def get_git_version():
    try:
        return subprocess.check_output(["git", "describe", "--always"]).decode().strip()
    except Exception:
        try:
            return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).decode().strip()
        except Exception:
            return "UNKNOWN"

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    version = get_git_version()
    print(f"-DMILIGHT_HUB_VERSION={version} {' '.join(sys.argv[1:])}")
