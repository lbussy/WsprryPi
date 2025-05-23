# Build Orchestration

Builds require the tag and branch to be consistent for use in the install script as well as the executable linking for compiler tags.  To do that, we will follow these steps:

1. Create or update your release branch

    ```bash
    git checkout -b 2.0_RC.1
    ```

2. Edit your source & install script
   - Bump any VERSION macros or install-script annotations to v1.2.3.
   - Make all other bug-fix or feature changes you need.

3. Commit just those edits

    ```bash
    git add src/ install.sh
    git commit -m "Prepare 2.0_RC.1 release"
    ```

4. Tag that commit (so your Makefile’s git describe/git rev-parse --abbrev-ref HEAD sees v1.2.3)

    ```bash
    git tag -a 2.0_RC.1 -m "Release Candidate 2.0 RC.1"
    ```

5. Build the binary

    ```bash
    ./release_tools/make_executables.sh
    ```

6. Stage the freshly built executable

    ```bash
    git add ./executables/
    ```

7. Amend your “2.0_RC.1” commit so it now includes the binary too

    ```bash
    git commit --amend --no-edit
    ```

8. Move the tag to point at the amended commit

    ```bash
    git tag -f 2.0_RC.1
    ```


9. Push your branch and tags

    ```bash
    git push origin 2.0_RC.1
    git push origin --force --tags
    ```

⸻

Why this order?

- Tag before build so your Makefile macros (e.g. via git describe --tags) pick up the new version.
- Build before adding the binary so the binary you commit really contains that tag.
- Amend + retag to keep everything (source edits, install-script note, and binary) in a single tagged commit.
- Force-push only on your release branch (never on main) so history rewriting is contained.

This guarantees that checking out v1.2.3 gives you exactly the source, the installer annotation, and the matching executable all in one shot.
