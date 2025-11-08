# CI/CD Pipeline Documentation

## Pipeline Architecture

```
                         GitHub Push/PR Event
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│                       CI Pipeline Stages                         │
└─────────────────────────────────────────────────────────────────┘
                                  │
        ┌─────────────────────────┼─────────────────────────┐
        │                         │                         │
        ▼                         ▼                         ▼
┌───────────────┐       ┌───────────────┐       ┌───────────────┐
│  Stage 1:     │       │  Stage 2:     │       │  Stage 3:     │
│  Lint & Format│──────►│  Build & Test │──────►│  Security     │
│               │       │               │       │  Scanning     │
└───────────────┘       └───────────────┘       └───────────────┘
  ↓ Jobs:                 ↓ Jobs:                 ↓ Jobs:
  • clang-format          • Linux (x64)           • CodeQL
  • clang-tidy            • macOS (x64/ARM)       • Snyk
  • cppcheck              • Windows (x64)         • Trivy
  • CMake format          • Run Tests             • Gitleaks
  • Shell lint            • Coverage              • Semgrep
  • YAML lint                                     • Bearer
  • Spell check                                   • Grype

        │                         │                         │
        └─────────────────────────┼─────────────────────────┘
                                  ▼
                        ┌───────────────────┐
                        │  Stage 4:         │
                        │  Quality Analysis │
                        └───────────────────┘
                          ↓ Jobs:
                          • SonarCloud
                          • Lizard (complexity)
                          • Valgrind (memory)

                                  │
                                  ▼
                    ┌───────────────────────────┐
                    │   All Checks Passed?      │
                    └───────────┬───────────────┘
                                │
                     ┌──────────┴──────────┐
                     │                     │
                  Yes│                     │No
                     ▼                     ▼
            ┌─────────────────┐   ┌─────────────────┐
            │  PR Approved    │   │  Fix Issues &   │
            │  Ready to Merge │   │  Re-run Pipeline│
            └─────────────────┘   └─────────────────┘
```

## Detailed Workflow

### Stage 1: Lint and Format Checks

**Purpose**: Fast failure for code style and format issues

```
Format Check (clang-format)
    │
    ├─► Checks C/C++ code formatting
    ├─► Uses clang-format-18
    └─► Runs in ~15 seconds

CMake Format Check
    │
    ├─► Validates CMake file formatting
    ├─► Uses gersemi
    └─► Runs in ~10 seconds

Static Analysis (clang-tidy)
    │
    ├─► Advanced C/C++ static analysis
    ├─► Checks for bugs, performance issues
    └─► Runs in ~50 seconds

Static Analysis (cppcheck)
    │
    ├─► C/C++ static analysis
    ├─► Finds bugs and undefined behavior
    └─► Runs in ~30 seconds

Shell Script Analysis (shellcheck)
    │
    ├─► Validates bash scripts
    └─► Runs in ~5 seconds

YAML Lint
    │
    ├─► Validates workflow YAML files
    └─► Runs in ~5 seconds

Spell Check (codespell)
    │
    ├─► Catches typos in code and docs
    └─► Runs in ~15 seconds
```

**Exit Criteria**: All linters pass, code is properly formatted

### Stage 2: Build and Test

**Purpose**: Verify code compiles and tests pass on all platforms

```
┌──────────────────────────────────────────────────────────┐
│              Multi-Platform Build Matrix                  │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  Ubuntu 22.04 (x86_64)                                   │
│    ├─► Build Debug & Release                             │
│    ├─► Run Unit Tests (CTest)                            │
│    ├─► Generate Coverage Report                          │
│    └─► Upload Artifacts                                  │
│                                                           │
│  macOS (Intel x64)                                       │
│    ├─► Build Universal Binary                            │
│    ├─► Run Unit Tests                                    │
│    ├─► Code Sign (if certificates available)             │
│    └─► Upload Artifacts                                  │
│                                                           │
│  macOS (Apple Silicon ARM64)                             │
│    ├─► Build Universal Binary                            │
│    ├─► Run Unit Tests                                    │
│    └─► Upload Artifacts                                  │
│                                                           │
│  Windows (x64)                                           │
│    ├─► Build Debug & Release                             │
│    ├─► Run Unit Tests                                    │
│    └─► Upload Artifacts                                  │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

**Build Process**:
```
Install Dependencies
    │
    ├─► libobs-dev
    ├─► libcurl-dev
    ├─► libjansson-dev
    └─► CMake, Ninja
    │
    ▼
Configure CMake
    │
    ├─► -DCMAKE_BUILD_TYPE=Release
    ├─► -DENABLE_TESTING=ON
    └─► -DENABLE_COVERAGE=ON (Linux only)
    │
    ▼
Build Project
    │
    ├─► ninja build
    └─► ~2 minutes per platform
    │
    ▼
Run Tests
    │
    ├─► Unit tests (CTest)
    ├─► API tests
    ├─► Config tests
    └─► Multistream tests
    │
    ▼
Generate Reports
    │
    ├─► Coverage (Linux)
    ├─► Test Results (JUnit XML)
    └─► Build Artifacts
```

**Exit Criteria**: All platforms build successfully, all tests pass

### Stage 3: Security Scanning

**Purpose**: Identify security vulnerabilities and secrets

```
┌─────────────────────────────────────────────────────────┐
│                  Security Scanners                       │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  CodeQL (GitHub Native)                                 │
│    ├─► Deep semantic analysis                           │
│    ├─► Queries: security-extended + quality             │
│    ├─► Detects: SQL injection, XSS, buffer overflow     │
│    └─► Results → GitHub Security Tab                    │
│                                                          │
│  Snyk                                                   │
│    ├─► Dependency vulnerability scanning               │
│    ├─► Severity threshold: high                        │
│    ├─► SARIF output                                    │
│    └─► Results → GitHub Security Tab                    │
│                                                          │
│  Trivy (Aqua Security)                                  │
│    ├─► Filesystem vulnerability scanner                │
│    ├─► Detects: HIGH, CRITICAL                         │
│    ├─► Scans dependencies and code                     │
│    └─► Results → GitHub Security Tab                    │
│                                                          │
│  Gitleaks                                               │
│    ├─► Secret scanning (API keys, tokens)              │
│    ├─► Full git history scan                           │
│    └─► Prevents credential leaks                        │
│                                                          │
│  Semgrep                                                │
│    ├─► Pattern-based code analysis                     │
│    ├─► Config: auto (community rules)                  │
│    ├─► Detects: insecure patterns                      │
│    └─► Results → GitHub Security Tab                    │
│                                                          │
│  Bearer                                                 │
│    ├─► Data security and privacy scanner               │
│    ├─► Detects: PII handling issues                    │
│    ├─► With SARIF validation                           │
│    └─► Results → GitHub Security Tab                    │
│                                                          │
│  Grype (Anchore)                                        │
│    ├─► Vulnerability scanner                            │
│    ├─► Severity cutoff: high                           │
│    └─► Results → GitHub Security Tab                    │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

**Security Flow**:
```
PR/Push Event
    │
    ├─► Run 7 security scanners in parallel
    │
    ├─► Generate SARIF reports
    │
    ├─► Upload to GitHub Security Tab
    │
    ├─► Check for CRITICAL/HIGH findings
    │
    └─► Flag PR if critical issues found
```

**Exit Criteria**: No critical/high vulnerabilities, no secrets detected

### Stage 4: Code Quality Analysis

**Purpose**: Ensure code maintainability and performance

```
┌─────────────────────────────────────────────────────────┐
│               Code Quality Tools                         │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  SonarCloud                                             │
│    ├─► Comprehensive quality metrics                    │
│    ├─► Code coverage analysis                           │
│    ├─► Technical debt calculation                       │
│    ├─► Code smells detection                            │
│    ├─► Duplicate code detection                         │
│    └─► Quality gate: Pass/Fail                          │
│                                                          │
│  Lizard (Complexity Analysis)                           │
│    ├─► Cyclomatic complexity (CCN)                      │
│    ├─► Threshold: CCN < 15                              │
│    ├─► Function analysis                                │
│    ├─► HTML report generation                           │
│    └─► Artifact: lizard-report.html                     │
│                                                          │
│  Valgrind (Memory Analysis)                             │
│    ├─► Memory leak detection                            │
│    ├─► Invalid memory access                            │
│    ├─► Uninitialized value usage                        │
│    ├─► Runs tests with --leak-check=full                │
│    └─► Artifact: valgrind-output.txt                    │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

**Quality Metrics**:
```
SonarCloud Quality Gate
    │
    ├─► Coverage: > 80%
    ├─► Duplications: < 3%
    ├─► Maintainability Rating: A
    ├─► Reliability Rating: A
    ├─► Security Rating: A
    └─► Pass/Fail Decision

Complexity Check
    │
    ├─► Max CCN per function: 15
    ├─► Flag high-complexity functions
    └─► Suggest refactoring

Memory Safety
    │
    ├─► Zero memory leaks
    ├─► No invalid access
    └─► All tests pass under Valgrind
```

**Exit Criteria**: Quality gate passes, complexity acceptable, no memory leaks

## Release Workflow

```
Tag Created (v*.*.*)
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│              Release Pipeline Triggers                   │
└─────────────────────────────────────────────────────────┘
    │
    ├─► Build all platforms (Linux, macOS, Windows)
    │
    ├─► Run full test suite
    │
    ├─► Package binaries
    │   ├─► .tar.gz (Linux)
    │   ├─► .pkg (macOS)
    │   └─► .zip (Windows)
    │
    ├─► Generate checksums (SHA256)
    │
    ├─► Create GitHub Release
    │   ├─► Upload artifacts
    │   ├─► Generate release notes
    │   └─► Mark as latest/pre-release
    │
    └─► Notify (optional)
```

## Artifact Management

```
Build Artifacts
    │
    ├─► Build Outputs
    │   ├─► obs-polyemesis.so (Linux)
    │   ├─► obs-polyemesis.dylib (macOS)
    │   └─► obs-polyemesis.dll (Windows)
    │
    ├─► Test Results
    │   ├─► test-results.xml (JUnit)
    │   └─► coverage.xml (Cobertura)
    │
    ├─► Security Reports
    │   ├─► *.sarif files
    │   └─► Uploaded to GitHub Security
    │
    ├─► Quality Reports
    │   ├─► lizard-report.html
    │   ├─► valgrind-output.txt
    │   └─► sonarcloud results
    │
    └─► Retention: 30 days
```

## Performance Metrics

Typical pipeline execution times:

| Stage | Duration |
|-------|----------|
| Lint & Format | ~2 min |
| Build (all platforms) | ~8 min |
| Tests | ~3 min |
| Security Scanning | ~5 min |
| Quality Analysis | ~6 min |
| **Total** | **~25 min** |

## Branch Protection Rules

```
main branch
    │
    ├─► Require pull request
    │   ├─► Require 1 approval (recommended)
    │   └─► Dismiss stale reviews
    │
    ├─► Require status checks
    │   ├─► lint (all jobs)
    │   ├─► build-and-test (all platforms)
    │   ├─► security (CodeQL, Snyk, Trivy)
    │   └─► quality-analysis (SonarCloud)
    │
    ├─► Require branches up to date
    │
    ├─► Block force pushes
    │
    └─► Block deletions
```

## Monitoring and Debugging

### View Pipeline Status

```bash
# List recent runs
gh run list --limit 10

# View specific run
gh run view <run-id>

# View logs
gh run view <run-id> --log

# Download artifacts
gh run download <run-id>
```

### Failed Pipeline Debugging

```
1. Check which job failed:
   gh run view <run-id>

2. View detailed logs:
   gh run view <run-id> --log-failed

3. Download artifacts for inspection:
   gh run download <run-id>

4. Reproduce locally:
   - Use same CMake flags
   - Run same test commands
   - Check dependencies match CI
```

## Local Testing with act

```bash
# Install act (GitHub Actions locally)
brew install act  # macOS
# or
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Run lint workflow
act workflow_call -W .github/workflows/lint.yaml

# Run build workflow (Linux only)
act workflow_call -W .github/workflows/build-project.yaml -j ubuntu-build

# List available workflows
act -l
```

## Security Best Practices

1. **No secrets in code** - Gitleaks prevents this
2. **Dependency scanning** - Snyk, Trivy catch vulnerabilities
3. **Code analysis** - CodeQL, Semgrep find security bugs
4. **SARIF upload** - All findings go to Security tab
5. **Scheduled scans** - Security runs weekly on Monday

## Optimization Tips

### Speed Up Pipeline

1. **Use caching**:
   ```yaml
   - uses: actions/cache@v4
     with:
       path: build
       key: ${{ runner.os }}-build-${{ hashFiles('**/CMakeLists.txt') }}
   ```

2. **Parallel jobs**:
   - Lint jobs run in parallel
   - Build matrix runs concurrently
   - Security scanners run simultaneously

3. **Fail fast**:
   - Lint fails early (Stage 1)
   - Prevents unnecessary builds

### Reduce Costs

1. **Self-hosted runners** for heavy jobs
2. **Skip CI** for docs-only changes:
   ```
   git commit -m "docs: update readme [skip ci]"
   ```

3. **Conditional workflows**:
   ```yaml
   if: github.event_name == 'push' && github.ref == 'refs/heads/main'
   ```

---

For more information, see:
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Contributing Guide](../CONTRIBUTING.md)
- [Security Policy](../SECURITY.md)
