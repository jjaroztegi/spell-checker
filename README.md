# Spell Checker

A multi-threaded spell checker written in C++17. It reads text from stdin, checks against a dictionary, and outputs an HTML file with misspelled words highlighted.

## Quick Start (Windows)

Double-click **`build.bat`**.

This script will automatically:

1. Download the required dictionary.
2. Detect your compiler (Visual Studio or G++).
3. Build the project.
4. Run the test and open the resulting HTML report.

_Note: If the script closes immediately or fails, try running `build.bat` from the "Developer Command Prompt for Visual Studio"._

## Quick Start (Linux / macOS)

Use the provided Makefile:

```bash
make run          # Builds, downloads dictionary, runs test, opens HTML
```

Other commands:

```bash
make              # Build only
make test         # Run test
make clean        # Remove build artifacts
```

## Usage

```bash
spell_checker.exe <dictionary_file> < input.txt > output.html
```

## Manual Compilation

Requires a C++17 compliant compiler.

**MSVC (Visual Studio):**

```cmd
mkdir build
cl /std:c++17 /O2 /EHsc /Fe:build\spell_checker.exe src\spell-checker.cpp
```

**g++ / clang++:**

```bash
mkdir -p build
g++ -std=c++17 -O3 -pthread -o build/spell_checker.exe src/spell-checker.cpp
```

## Features

- **Performance:** Uses multi-threading and `std::string_view`
- **Validates:**
  - Standard dictionary words.
  - Possessives (`John's`, `students'`).
  - Contractions (`didn't`, `I'm`).
  - Numbers (`2020`, `3.14`) and decades (`1960s`).
  - Hyphenated compounds (`working-class`).
  - Decades (`1960s`, `mid-1970s`).
- **Invalidates:**
  - Words not in dictionary
  - Mixed alphanumeric strings (`h3llo`, `p34r`).
