# Quality Assurance & Security Services

Comprehensive guide to quality assurance, testing, linting, and security scanning services integrated into OBS Polyemesis.

---

## Table of Contents

1. [Overview](#overview)
2. [Integrated Services](#integrated-services)
3. [Code Quality](#code-quality)
4. [Security Scanning](#security-scanning)
5. [Testing Infrastructure](#testing-infrastructure)
6. [CI/CD Pipeline](#cicd-pipeline)
7. [Badge Status](#badge-status)
8. [Service Configuration](#service-configuration)
9. [Additional Recommended Services](#additional-recommended-services)
10. [Best Practices](#best-practices)

---

## Overview

OBS Polyemesis employs a multi-layered approach to quality assurance and security:

- **8 Static Analysis Tools** for code quality
- **5 Security Scanners** for vulnerability detection
- **3 Test Suites** with 100% pass rate
- **Automated CI/CD** on every push and pull request
- **Weekly Security Scans** for dependency vulnerabilities

---

## Integrated Services

### 🔍 Currently Implemented

| Service | Purpose | Status | Workflow |
|---------|---------|--------|----------|
| **clang-format** | Code formatting verification | ✅ Active | `.github/workflows/lint.yaml` |
| **clang-tidy** | Static analysis for C/C++ | ✅ Active | `.github/workflows/lint.yaml` |
| **cppcheck** | Additional static analysis | ✅ Active | `.github/workflows/lint.yaml` |
| **gersemi** | CMake formatting | ✅ Active | `.github/workflows/lint.yaml` |
| **shellcheck** | Shell script analysis | ✅ Active | `.github/workflows/lint.yaml` |
| **yamllint** | YAML file validation | ✅ Active | `.github/workflows/lint.yaml` |
| **markdownlint** | Markdown documentation | ✅ Active | `.github/workflows/lint.yaml` |
| **codespell** | Spell checking | ✅ Active | `.github/workflows/lint.yaml` |
| **Snyk** | Dependency vulnerability scanning | ✅ Active | `.github/workflows/security.yaml` |
| **CodeQL** | Semantic code analysis | ✅ Active | `.github/workflows/security.yaml` |
| **Trivy** | Container & filesystem scanning | ✅ Active | `.github/workflows/security.yaml` |
| **OSV Scanner** | Open source vulnerability DB | ✅ Active | `.github/workflows/security.yaml` |
| **Dependency Review** | PR dependency impact | ✅ Active | `.github/workflows/security.yaml` |
| **SonarCloud** | Code quality & coverage | ✅ Active | `.github/workflows/sonarcloud.yaml` |
| **CTest** | Unit & integration testing | ✅ Active | `.github/workflows/build-project.yaml` |
| **AddressSanitizer** | Memory leak detection | ⚙️ Optional | Build with `-DENABLE_ASAN=ON` |
| **gcovr** | Code coverage reporting | ⚙️ Optional | Build with `-DENABLE_COVERAGE=ON` |

---

## Code Quality

### Static Analysis

#### clang-tidy
**Purpose**: Deep static analysis for C/C++ code
**Configuration**: `.clang-tidy`
**Checks**:
- Bug-prone patterns
- CERT secure coding standards
- C++ Core Guidelines
- Performance optimizations
- Modernization suggestions
- Readability improvements

**Run Locally**:
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
find src -name '*.c' -o -name '*.cpp' | xargs clang-tidy -p build
```

#### cppcheck
**Purpose**: Additional static analysis
**Run Locally**:
```bash
cppcheck --enable=all --suppress=missingIncludeSystem -I src src/
```

#### SonarCloud
**Purpose**: Continuous code quality inspection
**Metrics Tracked**:
- Code smells
- Technical debt
- Security hotspots
- Code coverage
- Duplications
- Complexity

**Dashboard**: https://sonarcloud.io/project/overview?id=rainmanjam_obs-polyemesis

---

### Code Formatting

#### clang-format
**Purpose**: Enforce consistent C/C++ code style
**Configuration**: `.clang-format`
**Style**: Based on LLVM with customizations

**Auto-fix**:
```bash
find src tests -name '*.c' -o -name '*.cpp' -o -name '*.h' | \
  xargs clang-format -i
```

#### gersemi
**Purpose**: CMake code formatting
**Run Locally**:
```bash
find . -name 'CMakeLists.txt' -o -name '*.cmake' | xargs gersemi --in-place
```

---

### Documentation Quality

#### markdownlint
**Purpose**: Markdown documentation linting
**Configuration**: `.markdownlint.json`
**Run Locally**:
```bash
npx markdownlint '**/*.md'
```

#### codespell
**Purpose**: Spell checking across all files
**Run Locally**:
```bash
codespell --skip=".git,*.png,*.jpg,build"
```

---

## Security Scanning

### Vulnerability Detection

#### Snyk
**Purpose**: Dependency and code vulnerability scanning
**Coverage**:
- Known CVEs in dependencies
- License compliance
- Code security issues

**Setup**:
1. Sign up at https://snyk.io
2. Add `SNYK_TOKEN` to repository secrets
3. Automated scans run weekly + on every push

#### CodeQL
**Purpose**: Semantic code security analysis
**Queries**:
- `security-extended`: Enhanced security checks
- `security-and-quality`: Combined analysis

**Languages**: C/C++

**Results**: GitHub Security tab > Code scanning alerts

#### Trivy
**Purpose**: Comprehensive vulnerability scanner
**Scans**:
- Filesystem vulnerabilities
- Configuration issues
- Secrets detection

**Severity Levels**: CRITICAL, HIGH

#### OSV Scanner
**Purpose**: Google's Open Source Vulnerability database
**Coverage**: All major ecosystems

#### Dependency Review
**Purpose**: PR-based dependency change analysis
**Triggers**: Pull requests only
**Fail Threshold**: Moderate severity

---

## Testing Infrastructure

### Test Suites

#### 1. Unit Tests (`obs-polyemesis-tests`)
**Framework**: Custom C test framework
**Coverage**:
- API client operations (5 tests)
- Configuration management (3 tests)
- Multistreaming logic (5 tests)

**Run Locally**:
```bash
cmake -B build -DENABLE_TESTING=ON
cmake --build build --config Release
cd build && ctest --output-on-failure
```

#### 2. Performance Benchmarks (`obs-polyemesis-benchmarks`)
**Metrics**:
- API client lifecycle performance
- Multistream configuration overhead
- Orientation detection speed (250,000+ ops/sec)
- Network latency

**Run Locally**:
```bash
./build/tests/Release/obs-polyemesis-benchmarks
```

#### 3. UI Tests (`obs-polyemesis-ui-tests`)
**Framework**: Qt Test
**Status**: Template stubs (for future implementation)

**Run Locally**:
```bash
./build/tests/Release/obs-polyemesis-ui-tests
```

### Code Coverage

**Tool**: gcovr
**Format**: SonarQube XML
**Enable**:
```bash
cmake -B build -DENABLE_COVERAGE=ON
cmake --build build
ctest --output-on-failure
gcovr --sonarqube coverage.xml -r .
```

### Memory Safety

**Tool**: AddressSanitizer (ASan)
**Purpose**: Detect memory leaks, buffer overflows, use-after-free
**Enable**:
```bash
cmake -B build -DENABLE_ASAN=ON
cmake --build build
./build/tests/Release/obs-polyemesis-tests
```

---

## CI/CD Pipeline

### Workflow Triggers

| Workflow | Trigger | Frequency |
|----------|---------|-----------|
| **Build & Test** | Push, PR | Every commit |
| **Lint** | Push, PR to main/develop | Every commit |
| **Security** | Push, PR, Schedule | Push + Weekly (Mondays) |
| **SonarCloud** | Push, PR to main/develop | Every commit |
| **Release** | Tag push (`v*.*.*`) | Manual |

### Quality Gates

All workflows must pass before merging to `main`:

1. ✅ **Build succeeds** on all platforms (macOS, Windows, Linux)
2. ✅ **All tests pass** (13/13)
3. ✅ **Code formatting** matches standards
4. ✅ **No high/critical security vulnerabilities**
5. ✅ **Static analysis** passes without errors
6. ✅ **CMake/YAML/Markdown/Shell** linting passes

---

## Badge Status

Add these badges to your README.md:

```markdown
[![Build Status](https://github.com/rainmanjam/obs-polyemesis/workflows/Build%20Project/badge.svg)](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/build-project.yaml)
[![Lint Status](https://github.com/rainmanjam/obs-polyemesis/workflows/Lint%20and%20Code%20Quality/badge.svg)](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/lint.yaml)
[![Security Scan](https://github.com/rainmanjam/obs-polyemesis/workflows/Security%20Scanning/badge.svg)](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/security.yaml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=rainmanjam_obs-polyemesis&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=rainmanjam_obs-polyemesis)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=rainmanjam_obs-polyemesis&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=rainmanjam_obs-polyemesis)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=rainmanjam_obs-polyemesis&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=rainmanjam_obs-polyemesis)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=rainmanjam_obs-polyemesis&metric=coverage)](https://sonarcloud.io/summary/new_code?id=rainmanjam_obs-polyemesis)
```

---

## Service Configuration

### Required Secrets

Add these to your GitHub repository settings (Settings → Secrets and variables → Actions):

| Secret | Purpose | Where to Get |
|--------|---------|--------------|
| `SNYK_TOKEN` | Snyk authentication | https://snyk.io/account |
| `SONAR_TOKEN` | SonarCloud authentication | https://sonarcloud.io/account/security |

### SonarCloud Setup

1. Go to https://sonarcloud.io
2. Sign in with GitHub
3. Import repository: `rainmanjam/obs-polyemesis`
4. Generate token: Account → Security → Generate Tokens
5. Add token to GitHub secrets as `SONAR_TOKEN`
6. Configure project:
   - Organization: `rainmanjam`
   - Project Key: `rainmanjam_obs-polyemesis`

### Snyk Setup

1. Go to https://snyk.io
2. Sign in with GitHub
3. Authorize Snyk to access repositories
4. Generate API token: Account Settings → API Token
5. Add token to GitHub secrets as `SNYK_TOKEN`

---

## Additional Recommended Services

### Highly Recommended

#### 1. **Codecov.io**
- **Purpose**: Code coverage tracking and visualization
- **Integration**: Free for open source
- **Setup**: https://about.codecov.io/
- **Benefits**:
  - Coverage trends over time
  - PR coverage comments
  - Coverage diff visualization

#### 2. **LGTM (by GitHub)**
- **Purpose**: Continuous security analysis
- **Integration**: GitHub Advanced Security
- **Benefits**: Deep code analysis, automated security fixes

#### 3. **Coverity Scan**
- **Purpose**: Static analysis specifically for open source
- **Integration**: Free for open source projects
- **Setup**: https://scan.coverity.com/
- **Benefits**: Industry-leading static analysis

#### 4. **Renovate Bot**
- **Purpose**: Automated dependency updates
- **Integration**: GitHub App
- **Setup**: https://github.com/apps/renovate
- **Benefits**:
  - Automated PR creation for updates
  - Smart scheduling
  - Grouped updates

#### 5. **DeepSource**
- **Purpose**: Automated code review
- **Integration**: Free for open source
- **Setup**: https://deepsource.io/
- **Benefits**:
  - 900+ code quality checks
  - Security analysis
  - Automated fixes

### Worth Considering

#### 6. **FOSSA**
- **Purpose**: License compliance scanning
- **Use Case**: Enterprise adoption, license auditing

#### 7. **WhiteSource (Mend)**
- **Purpose**: Open source security & license compliance
- **Use Case**: Enterprise-grade security

#### 8. **Veracode**
- **Purpose**: Application security testing
- **Use Case**: Enterprise security requirements

#### 9. **Checkmarx**
- **Purpose**: SAST (Static Application Security Testing)
- **Use Case**: Comprehensive security audits

#### 10. **GitGuardian**
- **Purpose**: Secrets detection
- **Integration**: GitHub App
- **Setup**: https://www.gitguardian.com/
- **Benefits**: Prevent credential leaks

#### 11. **Better Code Hub**
- **Purpose**: Code quality metrics
- **Focus**: Maintainability assessment

#### 12. **Code Climate**
- **Purpose**: Engineering intelligence platform
- **Metrics**: Maintainability, test coverage, velocity

---

## Best Practices

### Local Development

**Before Committing**:
```bash
# Format code
./check-format.sh

# Run tests
cmake -B build -DENABLE_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure

# Run static analysis
find src -name '*.c' -o -name '*.cpp' | xargs clang-tidy -p build

# Check for common issues
cppcheck --enable=all -I src src/
```

### Pull Request Checklist

- [ ] All CI/CD checks pass
- [ ] Code is properly formatted
- [ ] Tests added for new features
- [ ] Documentation updated
- [ ] No new security vulnerabilities
- [ ] SonarCloud quality gate passes
- [ ] Code coverage maintained or improved

### Security Best Practices

1. **Regular Dependency Updates**: Weekly automated scans
2. **Immediate CVE Response**: Act on HIGH/CRITICAL within 7 days
3. **Code Review**: All PRs require review
4. **Signed Commits**: Recommended for maintainers
5. **Secret Scanning**: Never commit credentials

### Continuous Improvement

- **Weekly**: Review security scan results
- **Monthly**: Analyze code coverage trends
- **Quarterly**: Review and update quality standards
- **Yearly**: Evaluate new QA/security tools

---

## Metrics & Goals

### Current Metrics
- **Test Coverage**: Tracked by SonarCloud
- **Static Analysis**: 0 errors, minimal warnings
- **Security Vulnerabilities**: 0 HIGH/CRITICAL
- **Code Smells**: Minimal (tracked by SonarCloud)
- **Technical Debt**: < 1% (SonarCloud metric)

### Quality Goals
- **Code Coverage**: > 80%
- **Duplication**: < 3%
- **Complexity**: Average < 10 per function
- **Security Rating**: A
- **Maintainability Rating**: A

---

## Troubleshooting

### Common Issues

**Local tests fail with code signing errors (macOS)**:
- Expected behavior on macOS with Homebrew libraries
- Tests will pass in CI/CD
- Use `codesign --remove-signature` on test binaries if needed

**SonarCloud build fails**:
- Ensure `SONAR_TOKEN` is set in repository secrets
- Check organization and project key match
- Verify build-wrapper output directory exists

**Snyk scan fails**:
- Verify `SNYK_TOKEN` is valid
- Check token has correct permissions
- Ensure Snyk supports C/C++ analysis

---

## Support & Resources

- **GitHub Actions Docs**: https://docs.github.com/actions
- **SonarCloud Docs**: https://docs.sonarcloud.io/
- **Snyk Docs**: https://docs.snyk.io/
- **clang-tidy Checks**: https://clang.llvm.org/extra/clang-tidy/checks/list.html
- **CodeQL Queries**: https://github.com/github/codeql

---

## Contributing

When adding new QA/security services:

1. Document configuration in this file
2. Update CI/CD workflows
3. Add status badges to README.md
4. Test locally before committing
5. Update troubleshooting section if needed

---

**Last Updated**: 2025-11-08
**Maintained By**: OBS Polyemesis Team
