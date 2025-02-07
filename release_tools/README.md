<!-- omit in toc -->
# WsprryPi Release Scripts Overview

- [Components](#components)
  - [`copyexe.sh`](#copyexesh)
    - [Purpose](#purpose)
    - [Key Features](#key-features)
    - [Usage](#usage)
    - [Test Cases](#test-cases)
  - [`clean_whitespace.sh`](#clean_whitespacesh)
    - [Purpose](#purpose-1)
    - [Key Features](#key-features-1)
    - [Usage](#usage-1)
    - [Test Cases](#test-cases-1)
  - [`release.py`](#releasepy)
    - [Key Features](#key-features-2)
    - [Usage](#usage-2)
    - [Configurable Parameters (via `release_config.py`)](#configurable-parameters-via-release_configpy)
  - [`get_semantic_version.sh`](#get_semantic_versionsh)
  - [`release_config.py`](#release_configpy)
    - [Purpose](#purpose-2)
    - [Key Configuration Settings](#key-configuration-settings)
    - [Example Configuration](#example-configuration)
  - [`rename_branch.sh`](#rename_branchsh)
    - [Purpose](#purpose-3)
    - [Key Features](#key-features-3)
    - [Usage](#usage-3)
    - [Test Cases](#test-cases-2)
  - [`get_semantic_version.py`](#get_semantic_versionpy)
    - [Purpose](#purpose-4)
    - [Key Features](#key-features-4)
    - [Usage](#usage-4)
    - [Test Cases](#test-cases-3)
  - [`copysite.sh`](#copysitesh)
    - [Purpose](#purpose-5)
    - [Key Features](#key-features-5)
    - [Usage](#usage-5)
    - [Test Cases](#test-cases-4)
  - [`update_versions.py`](#update_versionspy)
    - [Purpose](#purpose-6)
    - [Key Features](#key-features-6)
    - [Usage](#usage-6)
    - [Test Cases](#test-cases-5)
  - [`copydocs.sh`](#copydocssh)
    - [Purpose](#purpose-7)
    - [Key Features](#key-features-7)
    - [Usage](#usage-7)
    - [Test Cases](#test-cases-6)
- [Conclusion](#conclusion)


## Components

- `TODO.md`:
- `README.md`:
- `developer_notes.md`:
- `copyexe.sh`:
- `clean_whitespace.sh`:
- `release.py`:
- `get_semantic_version.sh`:
- `release_config.py`:
- `make_version.py`:
- `rename_branch.sh`:
- `get_semantic_version.py`:
- `copysite.sh`:
- `update_versions.py`:
- `copydocs.sh`:

My intent is to update copyright and release (Project, Tag, Branch) information and inject some of these into the compiled executable.

### `copyexe.sh`

#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---

### `clean_whitespace.sh`

#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---

### `release.py`

The `release.py` script updates project files with version and copyright information. It integrates with Git to retrieve the current repository's version details and updates the relevant headers in the project's source files.

#### Key Features

- **Header Update**: The script updates copyright years and version tags in supported files, ensuring they reflect the current project version.
- **Backup Creation**: Before making any changes, the script creates backups of files in the `scripts/bkp` directory.
- **Logging**: Comprehensive logs of all actions, including file updates, backups, and errors, are maintained for reference.
- **File Processing**: The script processes files in specified directories (`scripts`, `src`), updating headers in supported file types (e.g., `.py`, `.sh`, `.c`).
- **Git Integration**: The script pulls the current Git branch, tag, and commit hash to embed version information into the project files.
- **Conditional Compilation**: If correctly set up, the script can start a project compilation using `make` to ensure the project builds after header updates.

#### Usage

Run the `release.py` script to update headers in all files in the defined directories. Ensure that `release_config.py` is correctly configured for your specific environment.

```bash
python3 release.py
```

#### Configurable Parameters (via `release_config.py`)

- **Enable/Disable Backup**: Enable or disable the backup functionality.
- **Logging**: Control the logging level (e.g., DEBUG or INFO).
- **Directories to Process**: Define the directories to scan for files to update.
- **Supported File Extensions**: Specify the file types that the script should process.

---

### `get_semantic_version.sh`



### `release_config.py`

#### Purpose

The `release_config.py` file stores the configuration settings for the release process. This file controls how `release.py` operates, including which directories to scan, which files to update, and which specific operations (such as backup or compilation) to enable.

#### Key Configuration Settings

- **`ENABLE_BACKUP`**: If set to `True`, backups of the original files will be created before updating.
- **`ENABLE_LOGGING`**: Controls the logging of script actions. Set to `True` to log actions and errors.
- **`ENABLE_COMPILATION`**: If enabled, the script will trigger a `make` command to compile the project after file updates.
- **`ENABLE_COPY`**: The script will copy project executables to the specified location if enabled.
- **`DRY_RUN`**: If set to `True`, the script will simulate changes and print diffs without making any actual updates.
- **`SUPPORTED_EXTENSIONS`**: The script will process a list of file extensions (e.g., `.py`, `.sh`, `.c`).
- **`DIRECTORIES_TO_PROCESS`**: The directories to scan for files needing updates (e.g., `scripts`, `src`).
- **`LICENSE_PATTERNS`**: Patterns to identify license sections in the files for which the script should not modify.

#### Example Configuration

```python
class Config:
 ENABLE_BACKUP = True
 ENABLE_LOGGING = True
 ENABLE_COMPILATION = False
 ENABLE_COPY = False
 DRY_RUN = False
 SUPPORTED_EXTENSIONS = ['.py', '.sh', '.c', '.cpp', '.h']
 DIRECTORIES_TO_PROCESS = ['scripts', 'src']
 LICENSE_PATTERNS = ['MIT License', 'GPL', 'Apache License']
```

---

### `rename_branch.sh`
#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---

### `get_semantic_version.py`
#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---

### `copysite.sh`
#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---

### `update_versions.py`
#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---

### `copydocs.sh`
#### Purpose

#### Key Features

#### Usage



```bash
python3 foo.py
```

#### Test Cases

---


## Conclusion

These scripts automate updating the project headers, managing backups, and ensuring the project is ready for release. By adjusting the configuration in `release_config.py`, users can tailor the behavior of the release process to suit their needs. Always run `release_test.py` after modifying the main release script to ensure the updates function correctly.
