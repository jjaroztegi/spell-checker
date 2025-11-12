# Spell Checker

Multi-threaded spell checker written in C++17. Reads text from stdin, checks against a dictionary, and outputs HTML with misspelled words highlighted.

## Build

Requires C++17 compiler (g++, clang++, or MSVC).

```bash
make              # builds to build/spell_checker.exe
make run          # builds, runs on data/input.txt, opens output/test.html
make test         # runs test suite
make debug        # builds debug version
make clean        # removes build and output directories
```

Manual build:

```bash
# g++ or clang++
mkdir build output
g++ -std=c++17 -Wall -Wextra -O3 -pthread -o build/spell_checker.exe src/spell-checker.cpp

# MSVC (Visual Studio)
mkdir build output
cl /std:c++17 /O2 /EHsc /Fe:build\spell_checker.exe /Fo:build\ src\spell-checker.cpp
```

## Usage

```bash
spell_checker.exe dictionary.txt < input.txt > output.html
```

Example:

```bash
build/spell_checker.exe data/words_alpha.txt < data/input.txt > output/test.html
```

Dictionary file available at:
https://github.com/dwyl/english-words/blob/master/words_alpha.txt

## What it checks

**Valid:**

- Dictionary words (case-insensitive)
- Numbers: `2020`, `40,000`, `3.14`
- Possessives: `John's`, `students'`
- Contractions: `didn't`, `won't`, `I'm`
- Hyphenated compounds: `drop-in`, `working-class`
- Decades: `1960s`, `mid-1970s`

**Invalid:**

- Mixed alphanumeric: `p34r`, `h3llo`
- Words not in dictionary

## Output

HTML file with misspelled words highlighted in red. Original formatting preserved.

## Performance

Uses all available CPU threads. Optimized with:

- `std::string_view` for zero-copy string operations
- Pre-allocated vectors
- Minimal HTML output
- Binary stdout mode (Windows)

Tested with files up to 1GB.
