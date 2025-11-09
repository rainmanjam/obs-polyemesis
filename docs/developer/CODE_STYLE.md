# Code Style Guide

This document describes the code formatting standards for OBS Polyemesis and how to verify your code meets these standards.

## Overview

OBS Polyemesis uses automated formatters to ensure consistent code style:
- **CMake files**: Formatted with [gersemi](https://github.com/BlankSpruce/gersemi)
- **C/C++ files**: Formatted with [clang-format](https://clang.llvm.org/docs/ClangFormat.html)

## Quick Start

Before committing code, run the verification script:

```bash
./check-format.sh
```

If there are formatting issues, the script will tell you how to fix them automatically.

## Installation

### macOS

```bash
# Install formatters
pip3 install gersemi
brew install clang-format
```

### Linux (Ubuntu/Debian)

```bash
# Install formatters
pip3 install gersemi
sudo apt install clang-format
```

### Linux (Fedora/RHEL)

```bash
# Install formatters
pip3 install gersemi
sudo dnf install clang-tools-extra
```

### Windows

```bash
# Install formatters (using Python)
pip install gersemi

# Install clang-format
# Download from https://llvm.org/builds/
# Or use Chocolatey: choco install llvm
```

## Usage

### Check Code Style

Run the verification script to check all files:

```bash
./check-format.sh
```

**Output:**
- ✓ Green checkmarks = All files are properly formatted
- ✖ Red crosses = Files need formatting (with instructions to fix)

### Fix Formatting Issues

#### Automatic Fix (Recommended)

**CMake files:**
```bash
gersemi --in-place CMakeLists.txt cmake/**/*.cmake
```

**C/C++ files:**
```bash
clang-format -i src/*.c src/*.cpp src/*.h
```

#### Manual Fix

You can also fix formatting in your IDE:
- **VS Code**: Install "clang-format" and "cmake-format" extensions
- **CLion**: Built-in support, enable "Reformat Code" on save
- **Vim/Neovim**: Use plugins like `vim-clang-format` and `cmake-format.vim`

### Format Specific Files

**Single file:**
```bash
clang-format -i src/plugin-main.c
gersemi --in-place CMakeLists.txt
```

**Multiple files:**
```bash
clang-format -i src/restreamer-*.c
gersemi --in-place cmake/macos/*.cmake
```

## CI/CD Integration

The GitHub Actions workflow automatically checks code formatting:
- **Check Formatting workflow**: Runs on every push and pull request
- **Fails if**: Any file doesn't meet formatting standards
- **Passes if**: All files are properly formatted

You can see the formatting checks in:
- `.github/workflows/check-format.yaml`

## Style Configuration

### CMake Style (gersemi)

Default gersemi configuration is used. Key conventions:
- 2-space indentation
- Function names in lowercase with underscores
- Variables in UPPERCASE

### C/C++ Style (clang-format)

Uses the project's `.clang-format` configuration. Key conventions:
- Tab-based indentation (matching OBS Studio style)
- K&R-style braces
- 80-character line limit (when practical)
- Pointer/reference alignment to the variable name

## Pre-commit Hook (Optional)

You can automatically check formatting before each commit:

**Create `.git/hooks/pre-commit`:**
```bash
#!/bin/bash
# Pre-commit hook to check code formatting

./check-format.sh
if [ $? -ne 0 ]; then
    echo ""
    echo "Commit rejected due to formatting issues."
    echo "Run the suggested commands above to fix formatting."
    exit 1
fi
```

**Make it executable:**
```bash
chmod +x .git/hooks/pre-commit
```

## IDE Integration

### VS Code

**Install extensions:**
```bash
code --install-extension xaver.clang-format
code --install-extension cheshirekow.cmake-format
```

**Settings (`.vscode/settings.json`):**
```json
{
  "editor.formatOnSave": true,
  "C_Cpp.clang_format_style": "file",
  "cmake.format.allowOptionalArgumentIndentation": true
}
```

### CLion

1. Go to **Settings → Editor → Code Style → C/C++**
2. Click **Set from...** → **Predefined Style** → **LLVM**
3. Enable **Reformat Code** on save

### Vim/Neovim

**Using vim-plug:**
```vim
Plug 'rhysd/vim-clang-format'

" Auto-format on save
autocmd FileType c,cpp ClangFormatAutoEnable
```

## Common Issues

### Issue: gersemi not found

**Solution:**
```bash
pip3 install gersemi
# Or if using virtualenv:
source venv/bin/activate && pip install gersemi
```

### Issue: clang-format not found

**Solution (macOS):**
```bash
brew install clang-format
```

**Solution (Linux):**
```bash
sudo apt install clang-format  # Ubuntu/Debian
sudo dnf install clang-tools-extra  # Fedora
```

### Issue: "unknown command" warnings from gersemi

These warnings about custom CMake functions (like `_check_dependencies`, `target_install_resources`) are expected and can be ignored. They don't affect the formatting.

### Issue: Files still fail formatting check after running clang-format

Make sure you're using a compatible version:
```bash
clang-format --version  # Should be 14.0 or later
```

If the version is too old, update it:
```bash
brew upgrade clang-format  # macOS
```

## Best Practices

1. **Run before committing**: Always run `./check-format.sh` before committing
2. **Format incrementally**: Format files as you work on them
3. **IDE integration**: Set up auto-formatting in your IDE
4. **Pre-commit hooks**: Use the pre-commit hook to catch issues early
5. **CI/CD checks**: Let GitHub Actions catch any issues you missed

## Contributing

When contributing to OBS Polyemesis:

1. Ensure your code passes `./check-format.sh`
2. Run formatters before creating a pull request
3. Address any formatting issues raised in code review
4. Follow the existing code style in the project

## References

- [clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [gersemi documentation](https://github.com/BlankSpruce/gersemi)
- [OBS Studio coding guidelines](https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.rst)
