# Contributing to OBS Polyemesis

Thank you for your interest in contributing! This document explains our development workflow and guidelines.

## Branching Strategy

We follow **GitHub Flow** - a simple, branch-based workflow:

```
┌─────────────────────────────────────────────────────────────┐
│                         main branch                          │
│  (always deployable, protected, requires PR + CI passing)   │
└─────────────────────────────────────────────────────────────┘
     │              │              │              │
     ├─feature/A────┤              │              │
     │              │              │              │
     │              ├─feature/B────┤              │
     │              │              │              │
     │              │              ├─bugfix/C─────┤
     │              │              │              │
     ▼              ▼              ▼              ▼
  v1.0.0        v1.1.0        v1.1.1        v1.2.0
```

### Workflow Steps

1. **Create a feature branch from `main`**
   ```bash
   git checkout main
   git pull origin main
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes and commit**
   ```bash
   git add .
   git commit -m "Add feature: description"
   ```

3. **Push to GitHub**
   ```bash
   git push origin feature/your-feature-name
   ```

4. **Create a Pull Request**
   - Go to GitHub and create a PR from your branch to `main`
   - Fill in the PR template with a clear description
   - Link any related issues

5. **CI Pipeline runs automatically**
   - Linting and code formatting checks
   - Build on all platforms (Linux, macOS, Windows)
   - Unit tests
   - Security scanning (CodeQL, Snyk, Trivy, Gitleaks, Semgrep, etc.)
   - Code quality analysis (SonarCloud, Lizard, Valgrind)

6. **Address review feedback**
   - Make requested changes
   - Push new commits to your branch
   - CI runs again automatically

7. **Merge when approved and CI passes**
   - Squash and merge is preferred for clean history
   - Your branch will be automatically deleted

8. **Releases are tagged from `main`**
   - Maintainers create release tags (e.g., `v1.0.0`)
   - CI automatically builds and publishes release artifacts

## Branch Naming Conventions

- **Features**: `feature/short-description`
- **Bug fixes**: `bugfix/issue-number-description`
- **Hotfixes**: `hotfix/critical-issue-description`
- **Documentation**: `docs/what-you-are-documenting`
- **Refactoring**: `refactor/what-you-are-refactoring`

## Commit Message Guidelines

Follow conventional commits format:

```
type(scope): brief description

Detailed explanation if needed.

Fixes #issue-number
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks, dependency updates
- `ci`: CI/CD changes

**Examples**:
```
feat(api): add support for multiple restreamer endpoints
fix(config): handle null values in configuration parsing
docs(readme): update installation instructions
```

## Code Quality Standards

All code must pass:
- ✅ clang-format (code formatting)
- ✅ clang-tidy (static analysis)
- ✅ cppcheck (static analysis)
- ✅ Unit tests
- ✅ Security scans (no critical/high vulnerabilities)
- ✅ Complexity check (CCN < 15)

## Pull Request Checklist

Before submitting your PR, ensure:

- [ ] Code follows the project's coding style
- [ ] All tests pass locally
- [ ] Added tests for new features
- [ ] Updated documentation if needed
- [ ] Commit messages follow conventions
- [ ] No new compiler warnings
- [ ] Security scan passes
- [ ] PR description clearly explains the changes

## Development Setup

See [BUILDING.md](BUILDING.md) for detailed build instructions.

Quick start:
```bash
# Clone the repository
git clone https://github.com/rainmanjam/obs-polyemesis.git
cd obs-polyemesis

# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libobs-dev libcurl4-openssl-dev libjansson-dev

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

## Getting Help

- 📖 Read the [User Guide](USER_GUIDE.md)
- 🐛 Report bugs via [GitHub Issues](https://github.com/rainmanjam/obs-polyemesis/issues)
- 💬 Ask questions in [Discussions](https://github.com/rainmanjam/obs-polyemesis/discussions)

## Code of Conduct

Be respectful, inclusive, and constructive in all interactions.

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.
