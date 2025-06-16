import os
import sys
from subprocess import check_call, CalledProcessError

WEB_DIR = "web2"

try:
    os.chdir(WEB_DIR)
    print("Building Web UI...")
    if check_call(["npm", "install"]) != 0:
        print("Error installing npm packages", file=sys.stderr)
        exit(1)
    if check_call(["npm", "run", "build"]) != 0:
        print("Error building Web UI", file=sys.stderr)
        exit(1)
except FileNotFoundError as e:
    print("File not found during build:", e, file=sys.stderr)
    exit(1)
except OSError as e:
    print("Encountered OSError while building Web UI:", e, file=sys.stderr)
    if getattr(e, 'filename', None):
        print("Filename:", e.filename, file=sys.stderr)
    exit(1)
except CalledProcessError as e:
    print("Encountered CalledProcessError while building Web UI:", e, file=sys.stderr)
    exit(1)
