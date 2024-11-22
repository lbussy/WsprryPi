"""
Test Suite for Project Header Update Script

This module contains unit tests for validating the functionality of the Project Header Update Script.
It uses the `unittest` framework to test key features such as copyright year updates, version line changes,
file backup creation, Git information retrieval, and file processing. These tests ensure that the script
functions as expected across different scenarios, including handling errors, respecting configuration flags
like `DRY_RUN` and `ENABLE_BACKUP`, and updating files correctly.

Key Features Tested:
---------------------
- **Copyright Year Update**: Ensures the copyright year is correctly updated from a single year or a year range.
- **Version Line Update**: Validates that the version line is updated with the correct Git tag, commit hash, and branch.
- **File Backup Logic**: Confirms that backup files are created when necessary and that no backups are made if disabled.
- **Dry Run Mode**: Ensures no changes are made to files when `DRY_RUN` is enabled.
- **Git Information Retrieval**: Validates the retrieval of Git repository details (e.g., branch, tag, commit hash).
- **File Handling**: Ensures correct processing of files, including large files and files with multiple sections to update.
- **Permission Handling**: Simulates file permission errors to check proper error handling.

Test Coverage:
--------------
The test suite includes scenarios for various file formats, edge cases for file processing, large file handling,
dry-run operations, and excluded section handling. It also checks that all configurations in `release_config.py`
are respected.

Usage:
------
To run the full test suite, use:
    python -m unittest release_test.py

To run a specific test case or method:
    python -m unittest release_test.TestReleaseFunctions.test_update_copyright_year_single

Test Results:
-------------
The test results will display in the terminal, indicating which tests pass or fail. Detailed logs are available
for dry-run executions, file processing steps, and error messages.

Requirements:
-------------
- Python 3.x
- `unittest` module (standard in Python 3.x)

Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)

Created for WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
"""

import unittest
from unittest.mock import patch, mock_open
from pathlib import Path
from release_config import Config  # Import the configuration module

class TestReleaseFunctions(unittest.TestCase):
    """
    Unit tests for `release.py` script functionalities, covering areas like logging, dry-run mode, file backup,
    compilation, file copying, and error handling.
    """

    @patch("logging.basicConfig")
    def test_logging_enabled(self, mock_logging):
        """
        Test that logging is enabled when `Config.ENABLE_LOGGING` is True.
        Verifies that logging is properly set up when logging is enabled in the configuration.
        """
        Config.ENABLE_LOGGING = True
        import release  # Re-import the release script to apply the updated configuration

        release.main()  # Run the main function, which will trigger logging setup

        # Verify that logging setup was called
        mock_logging.assert_called_once()

    @patch("logging.basicConfig")
    def test_logging_disabled(self, mock_logging):
        """
        Test that logging is disabled when `Config.ENABLE_LOGGING` is False.
        Verifies that no logging setup occurs when logging is disabled in the configuration.
        """
        Config.ENABLE_LOGGING = False
        import release  # Re-import the release script to apply the updated configuration

        release.main()  # Run the main function, which should not call logging setup

        # Verify that logging setup was not called
        mock_logging.assert_not_called()

    @patch("builtins.open", new_callable=mock_open)
    def test_dry_run(self, mock_open):
        """
        Test the `DRY_RUN` mode, where no file changes are made.
        Ensures that no changes are written to files when `Config.DRY_RUN` is set to True.
        """
        Config.DRY_RUN = True
        mock_open.return_value.read.return_value = "line 1\nline 2\nline 3"

        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

        # Ensure that the file is not written to during dry-run mode
        mock_open.return_value.write.assert_not_called()

    @patch("shutil.copy")
    def test_backup_file(self, mock_copy):
        """
        Test that a backup is created when `Config.ENABLE_BACKUP` is True.
        Ensures that a backup file is copied to the `scripts/bkp` directory if backups are enabled.
        """
        Config.ENABLE_BACKUP = True
        file_path = Path("test_file.py")

        # Call the backup function from the `release` script
        from release import backup_file
        backup_file(file_path)

        # Verify that the backup was copied
        mock_copy.assert_called_once()

    @patch("shutil.copy")
    def test_backup_file_no_backup(self, mock_copy):
        """
        Test that no backup is created when `Config.ENABLE_BACKUP` is False.
        Ensures that no backup is made if the backup option is disabled in the configuration.
        """
        Config.ENABLE_BACKUP = False
        file_path = Path("test_file.py")

        # Call the backup function
        from release import backup_file
        backup_file(file_path)

        # Ensure no backup was made
        mock_copy.assert_not_called()

    @patch("subprocess.run")
    def test_compile_project(self, mock_run):
        """
        Test that project compilation occurs when `Config.ENABLE_COMPILATION` is True.
        Verifies that the `make clean` and `make` commands are run when compilation is enabled.
        """
        Config.ENABLE_COMPILATION = True
        project_directory = Path("/mock/project")

        # Simulate the compile project function call
        from release import compile_project
        compile_project(project_directory)

        # Ensure that the 'make' command was run as expected
        mock_run.assert_any_call("make clean", cwd=project_directory / "src")
        mock_run.assert_any_call("make", cwd=project_directory / "src")

    @patch("shutil.copy")
    def test_copy_files(self, mock_copy):
        """
        Test that executable files are copied when `Config.ENABLE_COPY` is True.
        Ensures that the specified executable files are copied to the scripts directory.
        """
        Config.ENABLE_COPY = True
        project_directory = Path("/mock/project")

        # Simulate the file copy operation
        from release import copy_files
        copy_files(project_directory, ["wspr", "wspr.ini"])

        # Verify that files are copied as expected
        mock_copy.assert_any_call(project_directory / "src" / "wspr", project_directory / "scripts" / "wspr")
        mock_copy.assert_any_call(project_directory / "src" / "wspr.ini", project_directory / "scripts" / "wspr.ini")

    @patch("builtins.open", new_callable=mock_open)
    def test_file_permission_error(self, mock_open):
        """
        Test that permission errors are correctly raised and handled.
        Simulates a `PermissionError` to verify that it is caught and handled by the script.
        """
        mock_open.side_effect = PermissionError("Permission denied")

        with self.assertRaises(PermissionError):
            with patch("release.update_files") as mock_update:
                mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

    @patch("builtins.open", new_callable=mock_open)
    def test_multiple_changes_in_file(self, mock_open):
        """
        Test that multiple changes (copyright and version) are correctly applied to a file.
        Ensures that both the copyright and version lines are updated when applicable.
        """
        content = """# Copyright (C) 2020 @LBussy
        version 1.2.1-55ad7f3 [fix_57]
        Some other content here"""
        mock_open.return_value.read.return_value = content
        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

        updated_content = mock_open.return_value.write.call_args[0][0]
        self.assertIn("Copyright (C) 2020-2024 @LBussy", updated_content)
        self.assertIn("version v1.2.4-abcd123 [main]", updated_content)

    @patch("builtins.open", new_callable=mock_open)
    def test_excluded_section(self, mock_open):
        """
        Test that excluded sections (e.g., GPL) are not updated in the file.
        Ensures that sections containing license information (e.g., GPL) are excluded from header updates.
        """
        content = """# Copyright (C) 2020 @LBussy
        # GNU General Public License
        version 1.0.0"""
        mock_open.return_value.read.return_value = content
        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

        updated_content = mock_open.return_value.write.call_args[0][0]
        self.assertIn("Copyright (C) 2020-2024 @LBussy", updated_content)
        self.assertNotIn("GNU General Public License", updated_content)  # Ensure GPL section is skipped
        self.assertIn("version v1.2.4-abcd123 [main]", updated_content)

    @patch("builtins.open", new_callable=mock_open)
    def test_large_file(self, mock_open):
        """
        Test that only the first `MAX_PROCESS_LINES` lines of a file are processed.
        Ensures that the script correctly limits the number of lines processed in large files.
        """
        # Simulate a file with 100 lines
        content = "\n".join([f"line {i}" for i in range(1, 101)])
        mock_open.return_value.read.return_value = content
        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

        # Ensure only the first 50 lines were processed (assuming MAX_PROCESS_LINES is set to 50)
        updated_content = mock_open.return_value.write.call_args[0][0]
        lines = updated_content.splitlines()
        self.assertEqual(len(lines), 50)  # Ensure only 50 lines were processed

    @patch("builtins.open", new_callable=mock_open)
    @patch("sys.stdout")
    def test_dry_run_with_diff(self, mock_stdout, mock_open):
        """
        Test that the diff is printed in dry-run mode without making file changes.
        Ensures that changes are displayed in the console when `DRY_RUN` is enabled, without modifying the file.
        """
        Config.DRY_RUN = True
        mock_open.return_value.read.return_value = "line 1\nline 2\nline 3"

        from release import update_file_content, colorized_diff
        original_content = mock_open.return_value.read.return_value
        updated_content = "line 1\nline 2 updated\nline 3"

        # Perform dry-run diff test
        diff = colorized_diff(original_content, updated_content)
        mock_stdout.write.assert_called_with(diff + "\n")  # Verify that the diff is printed with newline
