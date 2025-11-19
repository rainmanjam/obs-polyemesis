# Makefile for OBS Polyemesis
# Comprehensive development workflow for multi-platform builds

.PHONY: help all build test clean install uninstall package \
        build-macos build-linux build-windows build-all \
        test-macos test-linux test-windows test-all \
        package-macos package-linux package-windows package-all \
        clean-macos clean-linux clean-windows clean-all \
        install-macos uninstall-macos \
        check syntax-check lint format \
        deps-macos deps-check \
        artifacts artifacts-clean \
        docker-build docker-clean \
        windows-sync windows-build windows-test windows-package \
        pre-commit pre-commit-install \
        test-docker coverage coverage-docker coverage-view integration-docker

# Configuration
BUILD_TYPE ?= Release
BUILD_DIR ?= build
VERBOSE ?= 0

# Colors for output
BLUE := \033[0;34m
GREEN := \033[0;32m
YELLOW := \033[1;33m
NC := \033[0m # No Color

##@ General

help: ## Display this help message
	@echo "$(BLUE)OBS Polyemesis - Development Makefile$(NC)"
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make $(GREEN)<target>$(NC)\n"} /^[a-zA-Z_0-9-]+:.*?##/ { printf "  $(GREEN)%-20s$(NC) %s\n", $$1, $$2 } /^##@/ { printf "\n$(BLUE)%s$(NC)\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

all: build test ## Build and test all platforms

##@ Building

build: build-macos ## Build for current platform (macOS by default)

build-macos: ## Build for macOS
	@echo "$(BLUE)Building for macOS...$(NC)"
	@BUILD_TYPE=$(BUILD_TYPE) BUILD_DIR=$(BUILD_DIR) VERBOSE=$(VERBOSE) \
		./scripts/macos-build.sh

build-linux: ## Build for Linux (via Docker/act)
	@echo "$(BLUE)Building for Linux...$(NC)"
	@act -W .github/workflows/create-packages.yaml -j build-linux $(if $(filter 1,$(VERBOSE)),--verbose,)

build-windows: windows-build ## Build for Windows (via SSH)

windows-build: ## Build for Windows (remote)
	@echo "$(BLUE)Building for Windows...$(NC)"
	@VERBOSE=$(VERBOSE) ./scripts/sync-and-build-windows.sh

build-all: ## Build for all platforms sequentially
	@echo "$(BLUE)Building for all platforms...$(NC)"
	@BUILD_TYPE=$(BUILD_TYPE) VERBOSE=$(VERBOSE) ./scripts/build-all-platforms.sh

##@ Testing

test: test-macos ## Run tests for current platform

test-macos: ## Run tests on macOS
	@echo "$(BLUE)Running macOS tests...$(NC)"
	@BUILD_DIR=$(BUILD_DIR) VERBOSE=$(VERBOSE) ./scripts/macos-test.sh

test-linux: ## Run tests on Linux (via Docker/act)
	@echo "$(BLUE)Running Linux tests...$(NC)"
	@act -W .github/workflows/test.yaml $(if $(filter 1,$(VERBOSE)),--verbose,)

test-windows: windows-test ## Run tests on Windows (via SSH)

windows-test: ## Run tests on Windows (remote)
	@echo "$(BLUE)Running Windows tests...$(NC)"
	@./scripts/windows-test.sh

test-all: ## Run tests on all platforms
	@echo "$(BLUE)Running tests on all platforms...$(NC)"
	@VERBOSE=$(VERBOSE) ./scripts/test-all-platforms.sh

##@ Docker Testing

test-docker: ## Run unit tests in Docker (isolated environment)
	@echo "$(BLUE)Running unit tests in Docker...$(NC)"
	@./scripts/run-unit-tests-docker.sh

coverage: coverage-docker ## Generate code coverage in Docker (alias)

coverage-docker: ## Generate code coverage in Docker
	@echo "$(BLUE)Generating coverage in Docker...$(NC)"
	@./scripts/run-coverage-docker.sh

coverage-view: ## Open coverage report in browser
	@echo "$(BLUE)Opening coverage report...$(NC)"
	@if [ -f coverage/html/index.html ]; then \
		open coverage/html/index.html || xdg-open coverage/html/index.html 2>/dev/null || echo "$(YELLOW)Open coverage/html/index.html manually$(NC)"; \
	else \
		echo "$(YELLOW)Coverage report not found. Run 'make coverage' first.$(NC)"; \
		exit 1; \
	fi

integration-docker: ## Run integration tests with Docker Compose
	@echo "$(BLUE)Running integration tests in Docker Compose...$(NC)"
	@./scripts/run-integration-docker-compose.sh

##@ Packaging

package: package-macos ## Create package for current platform

package-macos: ## Create macOS installer package
	@echo "$(BLUE)Creating macOS package...$(NC)"
	@BUILD_DIR=$(BUILD_DIR) VERBOSE=$(VERBOSE) ./scripts/macos-package.sh

package-windows: windows-package ## Create Windows installer package

windows-package: ## Create Windows package (remote)
	@echo "$(BLUE)Creating Windows package...$(NC)"
	@./scripts/windows-package.sh

package-all: ## Create packages for all platforms
	@echo "$(BLUE)Creating packages for all platforms...$(NC)"
	@$(MAKE) build-all
	@$(MAKE) package-macos
	@$(MAKE) package-windows
	@echo "$(GREEN)All packages created!$(NC)"

##@ Installation (macOS only)

install-macos: ## Install plugin to OBS on macOS
	@echo "$(BLUE)Installing to OBS...$(NC)"
	@if [ -f "$(BUILD_DIR)/Release/obs-polyemesis.so" ]; then \
		mkdir -p "$$HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis/bin"; \
		cp "$(BUILD_DIR)/Release/obs-polyemesis.so" "$$HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis/bin/"; \
		echo "$(GREEN)✓ Plugin installed to OBS$(NC)"; \
	elif [ -f "$(BUILD_DIR)/obs-polyemesis.so" ]; then \
		mkdir -p "$$HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis/bin"; \
		cp "$(BUILD_DIR)/obs-polyemesis.so" "$$HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis/bin/"; \
		echo "$(GREEN)✓ Plugin installed to OBS$(NC)"; \
	else \
		echo "$(YELLOW)Plugin not found. Build first with: make build$(NC)"; \
		exit 1; \
	fi

uninstall-macos: ## Uninstall plugin from OBS on macOS
	@echo "$(BLUE)Uninstalling from OBS...$(NC)"
	@rm -rf "$$HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis"
	@echo "$(GREEN)✓ Plugin uninstalled$(NC)"

install: install-macos ## Install plugin (macOS)

uninstall: uninstall-macos ## Uninstall plugin (macOS)

##@ Cleaning

clean: clean-macos ## Clean build artifacts for current platform

clean-macos: ## Clean macOS build artifacts
	@echo "$(BLUE)Cleaning macOS build...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

clean-linux: ## Clean Linux build artifacts (Docker cache)
	@echo "$(BLUE)Cleaning Linux build cache...$(NC)"
	@act clean || true
	@echo "$(GREEN)✓ Clean complete$(NC)"

clean-windows: ## Clean Windows build artifacts (remote)
	@echo "$(BLUE)Cleaning Windows build...$(NC)"
	@ssh windows-act "powershell -Command 'if (Test-Path C:\\Users\\rainm\\Documents\\GitHub\\obs-polyemesis\\build) { Remove-Item -Path C:\\Users\\rainm\\Documents\\GitHub\\obs-polyemesis\\build -Recurse -Force; Write-Host \"Cleaned\" }'"

clean-all: clean-macos clean-linux clean-windows ## Clean all platforms

clean-artifacts: ## Clean artifact directories
	@echo "$(BLUE)Cleaning artifacts...$(NC)"
	@rm -rf artifacts
	@echo "$(GREEN)✓ Artifacts cleaned$(NC)"

##@ Code Quality

check: syntax-check ## Run all code quality checks

syntax-check: ## Check bash script syntax
	@echo "$(BLUE)Checking bash syntax...$(NC)"
	@bash -n install.sh
	@bash -n uninstall.sh
	@bash -n diagnose.sh
	@for script in scripts/*.sh; do \
		echo "Checking $$script..."; \
		bash -n "$$script" || exit 1; \
	done
	@echo "$(GREEN)✓ All scripts have valid syntax$(NC)"

lint: ## Run shellcheck on bash scripts (if available)
	@echo "$(BLUE)Running shellcheck...$(NC)"
	@if command -v shellcheck >/dev/null 2>&1; then \
		for script in scripts/*.sh install.sh uninstall.sh diagnose.sh; do \
			echo "Linting $$script..."; \
			shellcheck -x "$$script" || true; \
		done; \
		echo "$(GREEN)✓ Linting complete$(NC)"; \
	else \
		echo "$(YELLOW)shellcheck not installed. Install with: brew install shellcheck$(NC)"; \
	fi

format: ## Check code formatting (trailing whitespace, etc.)
	@echo "$(BLUE)Checking code format...$(NC)"
	@! git grep -I --line-number '	\+$$' -- '*.c' '*.cpp' '*.h' '*.sh' || \
		(echo "$(YELLOW)Found trailing whitespace (see above)$(NC)" && false)
	@echo "$(GREEN)✓ Format check passed$(NC)"

##@ Dependencies

deps-check: ## Check if required dependencies are installed (macOS)
	@echo "$(BLUE)Checking dependencies...$(NC)"
	@echo "Checking cmake..."
	@command -v cmake >/dev/null 2>&1 || (echo "$(YELLOW)cmake not found. Install with: brew install cmake$(NC)" && exit 1)
	@echo "Checking pkg-config..."
	@command -v pkg-config >/dev/null 2>&1 || (echo "$(YELLOW)pkg-config not found. Install with: brew install pkg-config$(NC)" && exit 1)
	@echo "Checking OBS Studio..."
	@[ -d "/Applications/OBS.app" ] || echo "$(YELLOW)OBS Studio not found. Install with: brew install --cask obs$(NC)"
	@echo "$(GREEN)✓ Core dependencies found$(NC)"

deps-macos: ## Install macOS dependencies via Homebrew
	@echo "$(BLUE)Installing macOS dependencies...$(NC)"
	@brew install cmake ninja qt6 jansson curl pkg-config
	@brew install --cask obs
	@echo "$(GREEN)✓ Dependencies installed$(NC)"

##@ Artifacts

artifacts: ## Create artifacts directory structure
	@echo "$(BLUE)Creating artifacts directory structure...$(NC)"
	@mkdir -p artifacts/{macos,linux,windows}
	@echo "$(GREEN)✓ Artifacts directories created$(NC)"

artifacts-collect: artifacts ## Collect build artifacts from all platforms
	@echo "$(BLUE)Collecting artifacts...$(NC)"
	@# macOS
	@if [ -d "$(BUILD_DIR)/installers" ]; then \
		cp $(BUILD_DIR)/installers/*.pkg artifacts/macos/ 2>/dev/null || true; \
		cp $(BUILD_DIR)/installers/*.pkg.sha256 artifacts/macos/ 2>/dev/null || true; \
	fi
	@# Windows (fetch from remote)
	@./scripts/windows-package.sh --fetch 2>/dev/null || true
	@echo "$(GREEN)✓ Artifacts collected$(NC)"

##@ Windows Remote Operations

windows-sync: ## Sync repository to Windows machine
	@echo "$(BLUE)Syncing to Windows...$(NC)"
	@./scripts/sync-and-build-windows.sh --sync-only

##@ Docker/Act

docker-build: build-linux ## Build using Docker (alias for build-linux)

docker-clean: ## Clean Docker build cache
	@echo "$(BLUE)Cleaning Docker cache...$(NC)"
	@docker system prune -f
	@echo "$(GREEN)✓ Docker cache cleaned$(NC)"

##@ Git Hooks

pre-commit-install: ## Install pre-commit hook
	@echo "$(BLUE)Installing pre-commit hook...$(NC)"
	@mkdir -p .git/hooks
	@cp scripts/pre-commit .git/hooks/pre-commit
	@chmod +x .git/hooks/pre-commit
	@echo "$(GREEN)✓ Pre-commit hook installed$(NC)"

pre-commit: ## Run pre-commit checks manually
	@echo "$(BLUE)Running pre-commit checks...$(NC)"
	@./scripts/pre-commit

##@ Quick Commands

quick-build: ## Quick macOS debug build
	@BUILD_TYPE=Debug $(MAKE) build-macos

quick-test: ## Quick macOS test (build if needed)
	@./scripts/macos-test.sh --build-first

rebuild: clean build ## Clean and rebuild

release: ## Full release build and package for all platforms
	@BUILD_TYPE=Release $(MAKE) build-all
	@BUILD_TYPE=Release $(MAKE) package-all
	@echo "$(GREEN)✓ Release build complete!$(NC)"

##@ Information

info: ## Display build configuration
	@echo "$(BLUE)Build Configuration:$(NC)"
	@echo "  Build Type:   $(BUILD_TYPE)"
	@echo "  Build Dir:    $(BUILD_DIR)"
	@echo "  Verbose:      $(VERBOSE)"
	@echo ""
	@echo "$(BLUE)Platform Information:$(NC)"
	@echo "  Host OS:      $$(uname -s)"
	@echo "  Architecture: $$(uname -m)"
	@echo "  CPUs:         $$(sysctl -n hw.ncpu 2>/dev/null || echo 'N/A')"
	@echo ""
	@echo "$(BLUE)Tools:$(NC)"
	@echo "  CMake:        $$(command -v cmake >/dev/null 2>&1 && cmake --version | head -1 || echo 'Not found')"
	@echo "  Act:          $$(command -v act >/dev/null 2>&1 && act --version || echo 'Not found')"
	@echo "  ShellCheck:   $$(command -v shellcheck >/dev/null 2>&1 && shellcheck --version | head -2 | tail -1 || echo 'Not found')"

version: ## Display project version
	@if git rev-parse --git-dir > /dev/null 2>&1; then \
		VERSION=$$(git describe --tags --abbrev=0 2>/dev/null || git rev-parse --short HEAD); \
		echo "Version: $$VERSION"; \
	else \
		echo "Not a git repository"; \
	fi
