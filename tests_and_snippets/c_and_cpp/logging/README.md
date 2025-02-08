<!-- omit in toc -->
# LCBLog - A Thread-Safe C++ Logging Library

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/language-C%2B%2B17-blue)

<!-- omit in toc -->
## Table of Contents

- [📌 Overview](#-overview)
- [🚀 Features](#-features)
- [📂 Project Structure](#-project-structure)
- [🛠️ Installation \& Compilation](#️-installation--compilation)
  - [🔧 Prerequisites](#-prerequisites)
  - [💻 Building the Library (`liblcblog.a`)](#-building-the-library-liblcbloga)
  - [🔍 Running Tests](#-running-tests)
  - [🛠 Debug Build (with symbols)](#-debug-build-with-symbols)
  - [🧹 Clean Build Artifacts](#-clean-build-artifacts)
  - [🔎 Static Code Analysis (`cppcheck`)](#-static-code-analysis-cppcheck)
- [✏️ Usage Example](#️-usage-example)
- [📜 License](#-license)
- [👨‍💻 Author](#-author)
- [⭐ Contributing](#-contributing)


## 📌 Overview

**LCBLog** is a flexible and thread-safe C++ logging library that provides:

- Multiple **log levels** (`DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`)
- **Thread safety** using `std::mutex`
- **Timestamped logs** (optional)
- **Customizable output streams** (supports redirection)
- **Multi-line message handling**
- **Static library (`liblcblog.a`)** for easy integration

---

## 🚀 Features

- [✔] **Log Level Filtering** – Messages below the set log level are ignored.
- [✔] **Thread Safety** – Multiple threads can log safely using `std::mutex`.
- [✔] **Customizable Output** – Logs can be redirected to different streams.
- [✔] **Performance Optimized** – Efficient whitespace handling in log messages.
- [✔] **Automatic Formatting** – Ensures aligned and readable log output.

---

## 📂 Project Structure

``` text
📁 lcblog
├── 📜 lcblog.hpp    # Header file (public API)
├── 📜 lcblog.cpp    # Implementation file
├── 📜 lcblog.tpp    # Template definitions
├── 📜 main.cpp      # Test executable (debug mode only)
├── 📜 Makefile      # Build system
├── 📜 README.md     # Project documentation
└── 📜 LICENSE       # MIT License
```

---

## 🛠️ Installation & Compilation

### 🔧 Prerequisites

- C++17 or newer
- `g++` or `clang++`
- `make`

### 💻 Building the Library (`liblcblog.a`)

``` bash
make
```

This will compile lcblog.cpp and create liblcblog.a.

### 🔍 Running Tests

To compile and run the test executable:

``` bash
make test
./lcblog_test
```

### 🛠 Debug Build (with symbols)

``` bash
make debug
```

### 🧹 Clean Build Artifacts

``` bash
make clean
```

---

### 🔎 Static Code Analysis (`cppcheck`)

You can check the code for potential issues using:

``` bash
make lint
```

If cppcheck is not installed, the Makefile will prompt you to install it.

---

## ✏️ Usage Example

Include & Initialize LCBLog

``` bash
#include "lcblog.hpp"

int main() {
    LCBLog logger; // Uses std::cout (info) and std::cerr (errors) by default

    logger.setLogLevel(DEBUG);  // Set log level to DEBUG
    logger.enableTimestamps(true); // Enable timestamps in logs

    logger.logS(INFO, "Application started.");
    logger.logS(DEBUG, "Debugging enabled.");
    logger.logE(ERROR, "An error occurred.");

    return 0;
}
```

Expected Output (with timestamps enabled):

``` text
2025-02-08 12:34:56 UTC [INFO ] Application started.
2025-02-08 12:34:56 UTC [DEBUG] Debugging enabled.
2025-02-08 12:34:56 UTC [ERROR] An error occurred.
```

---

## 📜 License

This project is licensed under the MIT License. See [LICENSE.md](LICENSE.md) for details.

---

## 👨‍💻 Author

Lee C. Bussy – [@LBussy](https://github.com/lbussy)

For contributions, bug reports, or feature requests, feel free to open an issue or submit a pull request. 🚀

---

## ⭐ Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository.
2. Create a new feature branch (git checkout -b feature-branch).
3. Commit changes (git commit -m "Added new feature").
4. Push the branch (git push origin feature-branch).
5. Open a pull request.

---

🔗 References

- [C++ `std::mutex` Documentation](https://en.cppreference.com/w/cpp/thread/mutex)
- [GCC Compiler](https://gcc.gnu.org/)
- [Makefile Documentation](https://www.gnu.org/software/make/manual/make.html)
- [`cppcheck`](http://cppcheck.sourceforge.net/)
