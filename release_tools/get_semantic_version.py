#!/usr/bin/env python

import subprocess
import os

class SemanticVersion:
    def __init__(self, repo_org="lbussy", repo_name="wsprrypi", repo_title="Wsprry Pi", repo_branch="install_update", git_tag="1.3.0", debug=False):
        self.repo_org = repo_org
        self.repo_name = repo_name
        self.repo_title = repo_title
        self.repo_branch = repo_branch
        self.git_tag = git_tag
        self.debug = debug

    def debug_print(self, message):
        """Print debug message if debug is enabled."""
        if self.debug:
            caller_name = self.get_caller_name()
            print(f"[DEBUG] {caller_name}: {message}")

    def get_caller_name(self):
        """Get the name of the calling function."""
        import inspect
        stack = inspect.stack()
        caller = stack[2]
        return caller.function

    def debug_start(self):
        """Start debug process."""
        if self.debug:
            self.debug_print("Starting debug process.")

    def debug_end(self):
        """End debug process."""
        if self.debug:
            self.debug_print("Ending debug process.")

    def get_repo_branch(self):
        """Get the current repository branch."""
        self.debug_start()

        try:
            branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], stderr=subprocess.STDOUT).strip().decode('utf-8')
            if branch == "HEAD":
                branch = self.repo_branch
            self.debug_print(f"Retrieved branch: {branch}")
            return branch
        except subprocess.CalledProcessError:
            self.debug_print("Unable to determine Git branch. Returning 'unknown'.")
            return "unknown"
        finally:
            self.debug_end()

    def get_last_tag(self):
        """Get the most recent Git tag."""
        self.debug_start()

        try:
            tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0'], stderr=subprocess.STDOUT).strip().decode('utf-8')
            self.debug_print(f"Retrieved tag: {tag}")
        except subprocess.CalledProcessError:
            tag = self.git_tag
            self.debug_print(f"No tag found. Using fallback: {tag}")
        finally:
            self.debug_end()
        return tag

    def get_num_commits(self, tag):
        """Get number of commits since the last tag."""
        self.debug_start()

        try:
            commit_count = subprocess.check_output(['git', 'rev-list', '--count', f"{tag}..HEAD"], stderr=subprocess.STDOUT).strip().decode('utf-8')
            self.debug_print(f"Number of commits since {tag}: {commit_count}")
            return commit_count
        except subprocess.CalledProcessError:
            self.debug_print(f"Unable to count commits from {tag}. Returning 0.")
            return "0"
        finally:
            self.debug_end()

    def get_short_hash(self):
        """Get the short hash of the current Git commit."""
        self.debug_start()

        try:
            short_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], stderr=subprocess.STDOUT).strip().decode('utf-8')
            self.debug_print(f"Short hash: {short_hash}")
            return short_hash
        except subprocess.CalledProcessError:
            self.debug_print("No short hash available. Returning empty string.")
            return ""
        finally:
            self.debug_end()

    def get_dirty(self):
        """Check if there are uncommitted changes in the working directory."""
        self.debug_start()

        try:
            changes = subprocess.check_output(['git', 'status', '--porcelain'], stderr=subprocess.STDOUT).strip().decode('utf-8')
            dirty = "true" if changes else "false"
            self.debug_print(f"Dirty state: {dirty}")
            return dirty
        except subprocess.CalledProcessError:
            self.debug_print("Unable to check for dirty state. Returning 'false'.")
            return "false"
        finally:
            self.debug_end()

    def get_sem_ver(self):
        """Generate semantic version string."""
        self.debug_start()

        tag = self.get_last_tag()
        branch_name = self.get_repo_branch()
        num_commits = self.get_num_commits(tag)
        short_hash = self.get_short_hash()
        dirty = self.get_dirty()

        version_string = f"{tag}-{branch_name}"

        if int(num_commits) > 0:
            version_string += f"+{num_commits}"

        if short_hash:
            version_string += f".{short_hash}"

        if dirty == "true":
            version_string += "-dirty"

        self.debug_print(f"Generated version string: {version_string}")
        self.debug_end()

        return version_string

    def main(self):
        """Main function to run the script."""
        print(self.get_sem_ver())


if __name__ == "__main__":
    # Example usage:
    version_generator = SemanticVersion(debug=False)
    version_generator.main()
