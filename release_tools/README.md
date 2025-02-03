<!-- omit in toc -->
# WsprryPi Release Script Overview

- [Components](#components)
  - [`release.py`](#releasepy)
    - [Purpose](#purpose)
    - [Key Features](#key-features)
    - [Usage](#usage)
    - [Configurable Parameters (via `release_config.py`)](#configurable-parameters-via-release_configpy)
  - [`release_config.py`](#release_configpy)
    - [Purpose](#purpose-1)
    - [Key Configuration Settings](#key-configuration-settings)
    - [Example Configuration](#example-configuration)
  - [`release_test.py`](#release_testpy)
    - [Purpose](#purpose-2)
    - [Key Features](#key-features-1)
    - [Usage](#usage-1)
    - [Test Cases](#test-cases)
- [Conclusion](#conclusion)

## Components

- `release.py`: The main script responsible for automating the update of project headers, including version and copyright information, across various project files.
- `release_config.py`: This is the configuration file that holds all the customizable settings used by `release.py` to control the behavior of the release process.
- `release_test.py`: A test script designed to validate the functionality and correctness of `release.py`.

My intent is to update copyright and release (Project, Tag, Branch) information and inject some of these into the compiled executable.

### `release.py`

#### Purpose
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

### `release_test.py`

#### Purpose

`release_test.py` is a test script designed to validate the behavior of `release.py`. It ensures that the script functions correctly by performing various checks on the files and configurations.

#### Key Features

- **Unit Testing**: Tests key functionality in `release.py`, including file processing, backup creation, and version updating.
- **Dry Run Validation**: Performs a dry run to ensure the script correctly simulates updates before making changes.
- **Mocking**: This method uses mock data to simulate file changes and verify that the expected updates are made to the project files.

#### Usage

Run `release_test.py` to execute tests and verify that the `release.py` script behaves as expected. The developer should run this test after making changes to `release.py` to ensure no regressions.

```bash
python3 release_test.py
```

#### Test Cases

- **Test Backup Creation**: Verifies that the script creates a backup folder and files it backs files up before being modified.
- **Test Header Updates**: This check ensures that the version and copyright information in project files are updated correctly.
- **Test Logging**: Verifies that the logging mechanism works as intended (e.g., logging updates and errors).
- **Test Dry Run**: Ensures that when `DRY_RUN` is enabled, the script makes no changes but displays diffs.

---

## Conclusion

These scripts automate updating the project headers, managing backups, and ensuring the project is ready for release. By adjusting the configuration in `release_config.py`, users can tailor the behavior of the release process to suit their needs. Always run `release_test.py` after modifying the main release script to ensure the updates function correctly.
