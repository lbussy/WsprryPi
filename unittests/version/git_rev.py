#!/usr/bin/python

import subprocess

# Get Git project name
projcmd = "git rev-parse --show-toplevel"
try:
    project = subprocess.check_output(projcmd, shell=True).decode().strip()
    project = project.split("/")
    project = project[len(project)-1]
except:
    project = "unknown"

# Get 0.0.0 version from latest Git tag
tagcmd = "git describe --tags --abbrev=0 --always"
try:
    version = subprocess.check_output(tagcmd, shell=True).decode().strip()
except:
    version = "0.0.0"

# Get latest commit short from Git
revcmd = "git log --pretty=format:'%h' -n 1"
try:
    commit = subprocess.check_output(revcmd, shell=True).decode().strip()
except:
    commit = "0000000"

# Get branch name from Git
branchcmd = "git rev-parse --abbrev-ref HEAD"
try:
    branch = subprocess.check_output(branchcmd, shell=True).decode().strip()
except:
    branch = "unknown"

# Make all available for use in the macros
print("-D MAKE_SRC_NAM={0}".format(project))
print("-D MAKE_SRC_TAG={0}".format(version))
print("-D MAKE_SRC_REV={0}".format(commit))
print("-D MAKE_SRC_BRH={0}".format(branch))
