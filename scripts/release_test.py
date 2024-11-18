"""
Test Suite for Project Header Update Script

This module contains the test suite for validating the behavior of the project header update script.
It uses the `unittest` framework to test key functionalities such as copyright year updates, version line changes,
file backup logic, Git information retrieval, and more. The tests ensure that the script correctly processes different 
types of files, handles errors, and respects configuration flags like `DRY_RUN` and `ENABLE_BACKUP`.

Key Tests:
- Copyright Year Update: Ensures the copyright year is updated correctly from a single year or year range.
- Version Line Update: Ensures the version line is updated with the correct Git tag, commit hash, and branch information.
- Backup Logic: Validates the file backup mechanism, ensuring backups are created when necessary and skipped if they already exist.
- Dry Run Mode: Tests the behavior of the script when the `DRY_RUN` flag is set, ensuring no changes are written to files during testing.
- Git Information Retrieval: Tests the retrieval of Git repository information (such as branch, tag, and commit hash).
- File Handling: Validates that the script processes files correctly, including handling large files and files with multiple sections that need to be updated.
- Permission Errors: Simulates permission-related errors to ensure the script handles them gracefully.

Constants:
- `TESTED_FILE_NAME`: The name of the file being tested (default is "release.py").
   - This constant allows for easy changes to the file being tested without having to modify individual test cases.

Test Coverage:
The test suite covers edge cases for file parsing, backup handling, large file handling, and dry-run functionality.
It also includes tests for various file formats and permission errors.

Usage:
------
To run the full test suite, use the following command in the terminal:
    python -m unittest release_test.py

To run a specific test case or method, use this format:
    python -m unittest release_test.TestProjectFunctions.test_update_copyright_year_single

Test Results:
-------------
The test results will be displayed in the terminal, showing whether all tests passed or if any tests failed.
The output includes logs for dry-run executions, file processing steps, and error messages.

Requirements:
-------------
- Python 3.x
- `unittest` module (standard in Python 3.x)
"""

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.1-9e6e021 [new_release_proc].

import unittest
from unittest.mock import patch, mock_open, MagicMock
from datetime import datetime
from pathlib import Path
import logging
from release_config import Config  # Import the config module

class TestReleaseFunctions(unittest.TestCase):
    """
    Unit tests for the `release.py` script, testing key functionalities such as logging, dry-run mode,
    file backup, compilation, file copying, and error handling.
    """

    # Test the logging behavior
    @patch("logging.basicConfig")
    def test_logging_enabled(self, mock_logging):
        """
        Test that logging is enabled when `Config.ENABLE_LOGGING` is True.
        """
        Config.ENABLE_LOGGING = True
        import release  # Re-import to apply the new config setting
        
        release.main()  # Run the main function which will call logging

        # Verify logging was set up
        mock_logging.assert_called_once()

    @patch("logging.basicConfig")
    def test_logging_disabled(self, mock_logging):
        """
        Test that logging is not enabled when `Config.ENABLE_LOGGING` is False.
        """
        Config.ENABLE_LOGGING = False
        import release  # Re-import to apply the new config setting
        
        release.main()  # Run the main function which should not call logging

        # Verify logging was not set up
        mock_logging.assert_not_called()

    # Test Dry-Run mode: No file changes should happen
    @patch("builtins.open", new_callable=mock_open)
    def test_dry_run(self, mock_open):
        """
        Test that no file changes happen when `Config.DRY_RUN` is True.
        The dry-run mode ensures that the script only prints the changes without modifying files.
        """
        Config.DRY_RUN = True
        mock_open.return_value.read.return_value = "line 1\nline 2\nline 3"
        
        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

        # Ensure the file is not written to during dry run
        mock_open.return_value.write.assert_not_called()

    # Test file backup behavior when ENABLE_BACKUP is True
    @patch("shutil.copy")
    def test_backup_file(self, mock_copy):
        """
        Test that file backup is created when `Config.ENABLE_BACKUP` is True.
        """
        Config.ENABLE_BACKUP = True
        file_path = Path("test_file.py")

        # Call the backup function
        from release import backup_file
        backup_file(file_path)

        # Verify that the backup file was copied
        mock_copy.assert_called_once()

    # Test backup behavior when ENABLE_BACKUP is False
    @patch("shutil.copy")
    def test_backup_file_no_backup(self, mock_copy):
        """
        Test that no backup is made when `Config.ENABLE_BACKUP` is False.
        """
        Config.ENABLE_BACKUP = False
        file_path = Path("test_file.py")

        # Call the backup function
        from release import backup_file
        backup_file(file_path)

        # Ensure no backup was made
        mock_copy.assert_not_called()

    # Test compilation when ENABLE_COMPILATION is True
    @patch("subprocess.run")
    def test_compile_project(self, mock_run):
        """
        Test that project compilation happens when `Config.ENABLE_COMPILATION` is True.
        """
        Config.ENABLE_COMPILATION = True
        project_directory = Path("/mock/project")
        
        # Call the compile function
        from release import compile_project
        compile_project(project_directory)

        # Verify that 'make' command was run
        mock_run.assert_any_call("make clean", cwd=project_directory / "src")
        mock_run.assert_any_call("make", cwd=project_directory / "src")

    # Test file processing when ENABLE_COPY is True
    @patch("shutil.copy")
    def test_copy_files(self, mock_copy):
        """
        Test that executable files are copied when `Config.ENABLE_COPY` is True.
        """
        Config.ENABLE_COPY = True
        project_directory = Path("/mock/project")
        
        # Simulate copying of files
        from release import copy_files
        copy_files(project_directory, ["wspr", "wspr.ini"])

        # Ensure files are being copied
        mock_copy.assert_any_call(project_directory / "src" / "wspr", project_directory / "scripts" / "wspr")
        mock_copy.assert_any_call(project_directory / "src" / "wspr.ini", project_directory / "scripts" / "wspr.ini")

    # Test file permission error
    @patch("builtins.open", new_callable=mock_open)
    def test_file_permission_error(self, mock_open):
        """
        Test that permission errors are correctly raised and handled.
        """
        mock_open.side_effect = PermissionError("Permission denied")
        
        with self.assertRaises(PermissionError):
            with patch("release.update_files") as mock_update:
                mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")

    # Test multiple changes in one file
    @patch("builtins.open", new_callable=mock_open)
    def test_multiple_changes_in_file(self, mock_open):
        """
        Test that multiple changes (copyright and version) are correctly applied to a file.
        """
        content = """# Copyright (C) 2020 @LBussy
        version 1.0.0
        Some other content here"""
        mock_open.return_value.read.return_value = content
        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")
        
        updated_content = mock_open.return_value.write.call_args[0][0]
        self.assertIn("Copyright (C) 2020-2024 @LBussy", updated_content)
        self.assertIn("version v1.2.4-abcd123 [main]", updated_content)

    # Test excluded sections
    @patch("builtins.open", new_callable=mock_open)
    def test_excluded_section(self, mock_open):
        """
        Test that excluded sections (e.g., GPL) are not updated in the file.
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

    # Test large file handling
    @patch("builtins.open", new_callable=mock_open)
    def test_large_file(self, mock_open):
        """
        Test that only the first `MAX_PROCESS_LINES` lines of a file are processed.
        """
        # Simulate a file with 100 lines
        content = "\n".join([f"line {i}" for i in range(1, 101)])
        mock_open.return_value.read.return_value = content
        with patch("release.update_files") as mock_update:
            mock_update(Path("/mock/project"), "main", "v1.2.4", "abcd123")
        
        # Ensure only the first MAX_PROCESS_LINES (50) lines were processed
        updated_content = mock_open.return_value.write.call_args[0][0]
        lines = updated_content.splitlines()
        self.assertEqual(len(lines), 50)  # Ensure only 50 lines were processed

    # Test Dry Run and Diff output
    @patch("builtins.open", new_callable=mock_open)
    @patch("sys.stdout")
    def test_dry_run_with_diff(self, mock_stdout, mock_open):
        """
        Test that the diff is printed in dry-run mode without making file changes.
        """
        Config.DRY_RUN = True
        mock_open.return_value.read.return_value = "line 1\nline 2\nline 3"

        from release import update_file_content, colorized_diff
        original_content = mock_open.return_value.read.return_value
        updated_content = "line 1\nline 2 updated\nline 3"

        # Perform dry-run diff test
        diff = colorized_diff(original_content, updated_content)
        mock_stdout.write.assert_called_with(diff + "\n")  # Verify diff is printed with newline
