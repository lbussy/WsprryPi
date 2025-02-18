#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# -----------------------------------------------------------------------------
# @file release_config.sh
# @brief Configuration for release management
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-ui_features+55.c921e43-dirty
# @date 2025-02-18
# @copyright MIT License
#
# @license
# MIT License
#
# Copyright (c) 2023-2025 Lee C. Bussy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# -----------------------------------------------------------------------------

from pathlib import Path

class Config:
    DRY_RUN = True
    ENABLE_COMPILATION = True
    ENABLE_COPY = True
    DEBUG = True

    @staticmethod
    def get_license_exclusion_patterns():
        """
        Compile regex patterns for excluding license sections.
        """
        import re
        return [re.compile(pattern) for pattern in Config.LICENSE_PATTERNS]


def main():
    """
    Main function to list all properties of the Config class.
    """
    print("Listing all configuration properties of the Config class:")
    for attribute in dir(Config):
        if not attribute.startswith("__") and not callable(getattr(Config, attribute)):
            value = getattr(Config, attribute)
            print(f"{attribute}: {value}")


if __name__ == "__main__":
    main()
