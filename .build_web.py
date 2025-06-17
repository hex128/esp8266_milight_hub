WEB_DIR = "web2"

Import("env")

import subprocess
# Only run if we're actually building
if any(t in COMMAND_LINE_TARGETS for t in ["build", "upload", "program", "uploadfs"]):
    try:
        subprocess.check_call(["npm", "install"], cwd=WEB_DIR)
        subprocess.check_call(["npm", "run", "build"], cwd=WEB_DIR)
    except subprocess.CalledProcessError as e:
        print("Error during npm build process.")
        env.Exit(1)
