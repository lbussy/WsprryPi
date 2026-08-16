# Issue 415 Snapshot and Comparison Method

Status: Pre-migration procedure

Contract: [`../plans/submodule-absorption-contract.md`](../plans/submodule-absorption-contract.md)

Baseline: [`submodule-absorption-baseline.md`](submodule-absorption-baseline.md)

## Purpose

This procedure converts one reviewed gitlink at a time using only the exact
parent-recorded component tree. It prevents ignored files, generated output,
untracked files, and nested Git administration from entering the parent index.
It also produces a content-addressed comparison between the former component
tree and the staged ordinary tree before any Phase B adaptation.

Do not run these commands until the implementation task explicitly authorizes
working-tree and index changes. Substitute one reviewed component path at a
time. Stop on every failed assertion.

## Per-component preflight

Start from the repository root with the complete clean gate already passed:

```sh
component_path=src/INI-Handler
gitlink_sha=$(git ls-files -s -- "$component_path" | awk '$1 == "160000" {print $2}')
checked_out_sha=$(git -C "$component_path" rev-parse HEAD)
source_tree=$(git -C "$component_path" rev-parse 'HEAD^{tree}')
source_gitdir=$(git -C "$component_path" rev-parse --absolute-git-dir)

test -n "$gitlink_sha"
test "$checked_out_sha" = "$gitlink_sha"
test -z "$(git -C "$component_path" status --porcelain)"
```

Compare the resulting SHAs and tree OID with the baseline manifest. Record the
current URL, state, commit metadata, license, tags, ignored files, untracked
files, and tracked inventory before continuing. An empty porcelain status does
not make ignored artifacts eligible for import.

## Object-derived snapshot

Create a unique temporary preservation directory outside the checkout. Do not
reuse a directory from another component:

```sh
migration_tmp=$(mktemp -d /private/tmp/wsprrypi-issue415.XXXXXX)
archive_path="$migration_tmp/component.tar"

git -C "$component_path" archive --format=tar "$gitlink_sha" > "$archive_path"
tar -tf "$archive_path" > "$migration_tmp/archive-paths.txt"
shasum -a 256 "$archive_path"
```

Inspect `archive-paths.txt`. The archive must contain only paths tracked by the
recorded commit and must contain no `.git` file or directory. Preserve the
archive digest and path list as comparison evidence.

`git archive` is mandatory here. Do not copy the live component directory into
the parent as the import source.

## Expected raw tree

For every component except WSPR-Transmitter, the expected staged raw tree is the
exact source tree:

```sh
expected_tree="$source_tree"
```

WSPR-Transmitter has one approved tracked exclusion. Compute its expected tree
with a temporary index backed by the component object database:

```sh
expected_index="$migration_tmp/expected.index"
GIT_INDEX_FILE="$expected_index" git --git-dir="$source_gitdir" read-tree "$source_tree"
GIT_INDEX_FILE="$expected_index" git --git-dir="$source_gitdir" \
  update-index --force-remove -- src/.codex
expected_tree=$(GIT_INDEX_FILE="$expected_index" \
  git --git-dir="$source_gitdir" write-tree)
```

No other exclusion is implicit. A newly discovered exclusion requires review
and a contract update before conversion.

## Gitlink replacement

Preserve the complete live submodule directory before changing the parent
index. This retains its Git link and all ignored local artifacts for review
without exposing them to `git add`:

```sh
preserved_live="$migration_tmp/reviewed-live-submodule"
mv "$component_path" "$preserved_live"
git rm --cached -- "$component_path"
mkdir -p "$component_path"
tar -xf "$archive_path" -C "$component_path"
```

For WSPR-Transmitter only, preserve the approved exclusion outside the restored
tree rather than deleting it:

```sh
mv "$component_path/src/.codex" "$migration_tmp/excluded-src-.codex"
```

Do not delete `.git/modules` data. The former component object database remains
available as comparison evidence; optional administrative cleanup is outside
this migration.

Stage only the selected component path:

```sh
git add -A -- "$component_path"
```

## Content-addressed comparison

Write the current parent index tree and resolve the selected staged subtree:

```sh
parent_index_tree=$(git write-tree)
staged_tree=$(git rev-parse "$parent_index_tree:$component_path")
test "$staged_tree" = "$expected_tree"
```

Tree equality proves identical relative paths, modes, and blob content. For
WSPR-Transmitter it proves equality to the source tree minus only the approved
`src/.codex` exclusion.

Also verify the review representation and artifact boundary:

```sh
test -z "$(find "$component_path" -name .git -print)"
git diff --cached --raw -- "$component_path"
git diff --cached --check -- "$component_path"
git status --short --ignored -- "$component_path"
```

The raw diff must show deletion of the mode `160000` gitlink and additions of
ordinary files. No ignored, generated, untracked, or undeclared tool-state file
may be staged. Record the source tree, expected tree, staged tree, archive
digest, exclusion list, and diff summary in the migration evidence.

## Phase boundary and failure handling

Stop after the raw-tree comparison passes. Preserve `migration_tmp` and the
staged Phase A state until review evidence has been recorded. Do not start
license consolidation, documentation edits, build-name fixes, workflow moves,
or any other Phase B adaptation while claiming raw equivalence.

If any command or assertion fails:

- stop processing that component and do not continue to another component;
- do not reset, clean, stash, or improvise a rollback;
- preserve the temporary archive, expected index, original live submodule, and
  current parent index for diagnosis;
- report the exact failed command and observed SHAs or tree OIDs.

Restoration or retry must be separately reviewed against the preserved evidence.

## Disposable proof

This method was exercised on 2026-08-16 in a disposable repository using the
recorded WSPR-Transmitter commit. The source tree was
`cc0ca41ad8345369f5c9ff6725e6b03b7be64c40`. After removing only the approved
`src/.codex` path, both the independently computed expected tree and the staged
ordinary tree were `b57fc117775ecd8a171fd757d6de5418ae48f92f`. The raw staged
diff showed the mode `160000` deletion and ordinary file additions. No actual
WsprryPi gitlink or component working tree was changed by the proof.
