#!/usr/bin/python3

import subprocess, os
from fileinput import FileInput

def get_git():
    print("Getting environment info")
    # Get Git project name
    global project
    projcmd = "git rev-parse --show-toplevel"
    try:
        project = subprocess.check_output(projcmd, shell=True).decode().strip()
        project = project.split("/")
        project = project[len(project)-1]
    except:
        project = "unknown"

    # Get 0.0.0 version from latest Git tag
    global version
    tagcmd = "git describe --tags --abbrev=0 --always"
    try:
        version = subprocess.check_output(tagcmd, shell=True).decode().strip()
    except:
        version = "0.0.0"

    # Get latest commit short from Git
    global commit
    revcmd = "git log --pretty=format:'%h' -n 1"
    try:
        commit = subprocess.check_output(revcmd, shell=True).decode().strip()
    except:
        commit = "0000000"

    # Get branch name from Git
    global branch
    branchcmd = "git rev-parse --abbrev-ref HEAD"
    try:
        branch = subprocess.check_output(branchcmd, shell=True).decode().strip()
    except:
        branch = "unknown"


def replace_in_file(file, string, replace):
    global version
    print("Changing {} to version {}.".format(file, version))
    with FileInput(files=file, inplace=True) as f:
        for line in f:
            if string in line:
                line = string + "\"" + replace + "\"\n"
            print(line, end='')


def edit_files():
    global version
    global branch
    global project
    replace_in_file("install.sh", "VERSION=", version)
    replace_in_file("uninstall.sh", "VERSION=", version)
    replace_in_file("install.sh", "BRANCH=", branch)
    replace_in_file("uninstall.sh", "BRANCH=", branch)
    replace_in_file("shutdown-button.py", "# Created for " + project + " version ", version)


def compile():
    global project
    global version
    current_dir = os.getcwd()
    project_dir_command = "git rev-parse --show-toplevel"
    project_dir = subprocess.check_output(project_dir_command, shell=True).decode().strip()
    source_dir = project_dir + "/src"
    os.chdir(source_dir)
    print("Compiling {} version {}.".format(project, version))
    compile_command = "make clean && make"
    subprocess.check_output(compile_command, shell=True)
    os.chdir(current_dir)


def copy(file):
    current_dir = os.getcwd()
    project_dir_command = "git rev-parse --show-toplevel"
    project_dir = subprocess.check_output(project_dir_command, shell=True).decode().strip()
    source_dir = project_dir + "/src"
    copy_command = "cp -f " + source_dir + "/wspr " + current_dir
    print("Copying {} to {}.".format(file, current_dir))
    subprocess.check_output(copy_command, shell=True)


def main():
    get_git()
    edit_files()
    compile()
    copy("wspr")
    copy("wspr.ini")


if __name__=="__main__":
    main()
