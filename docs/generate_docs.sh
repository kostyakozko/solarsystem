#!/bin/bash
# Enhanced documentation generation for Solar System Suite
# Generates comprehensive API documentation with GitHub Pages deployment support

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOCS_DIR="docs"
API_DIR="$DOCS_DIR/api"
OUTPUT_DIR="$API_DIR/html"
DOXYGEN_CONFIG="$DOCS_DIR/Doxyfile"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BLUE}===============================NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Verify prerequisites
verify_prerequisites() {
    log_section "VERIFYING PREREQUISITES"

    # Check for required tools
    local required_tools=("doxygen" "dot")
    for tool in "${required_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_success "$tool: Available"
        else
            log_error "$tool: Not found"
            if [ "$tool" = "dot" ]; then
                log_info "Install graphviz package for diagram generation"
            fi
            exit 1
        fi
    done

    # Check Doxygen configuration
    if [ ! -f "$DOXYGEN_CONFIG" ]; then
        log_error "Doxygen configuration not found: $DOXYGEN_CONFIG"
        exit 1
    fi

    log_success "Prerequisites verified"
}

# Clean previous documentation
clean_docs() {
    log_section "CLEANING PREVIOUS DOCUMENTATION"

    if [ -d "$API_DIR" ]; then
        log_info "Removing existing API documentation..."
        rm -rf "$API_DIR"
        log_success "Cleaned API documentation directory"
    fi

    # Create fresh directories
    mkdir -p "$API_DIR"
    log_success "Created fresh documentation directories"
}

# Generate project statistics
generate_project_stats() {
    log_section "GENERATING PROJECT STATISTICS"

    local stats_file="$DOCS_DIR/project_stats.md"

    {
        echo "# Solar System Suite - Project Statistics"
        echo ""
        echo "Generated: $(date)"
        echo ""

        echo "## Code Statistics"
        echo ""

        # Count source files
        local cpp_files=$(find lib apps -name "*.cpp" | wc -l)
        local h_files=$(find lib apps -name "*.h" -o -name "*.hpp" | wc -l)
        local total_files=$((cpp_files + h_files))

        echo "- Source files: $cpp_files (.cpp)"
        echo "- Header files: $h_files (.h/.hpp)"
        echo "- Total files: $total_files"
        echo ""

        # Count lines of code
        local total_lines=$(find lib apps -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs wc -l | tail -1 | awk '{print $1}')
        echo "- Total lines of code: $total_lines"
        echo ""

        echo "## Library Structure"
        echo ""
        echo "- **solar_core**: Physics simulation engine"
        echo "- **solar_jpl**: JPL HORIZONS API integration"
        echo "- **solar_utils**: Shared utilities and argument parsing"
        echo "- **solar_test**: Comprehensive testing framework"
        echo ""

        echo "## Application Suite"
        echo ""
        echo "- **solar_system**: High-performance batch simulation"
        echo "- **solar_system_fetch**: Data management and caching"
        echo "- **solar_system_launcher**: Unified interface and coordinator"
        echo "- **solar_system_realtime**: Live solar system tracking"
        echo "- **solar_system_web**: Interactive web-based visualization"
        echo ""

        echo "## Test Coverage"
        echo ""
        local unit_tests=$(find tests/unit -name "*.cpp" | wc -l)
        local integration_tests=$(find tests/integration -name "*.cpp" | wc -l)
        local benchmark_tests=$(find tests/benchmarks -name "*.cpp" | wc -l)

        echo "- Unit tests: $unit_tests"
        echo "- Integration tests: $integration_tests"
        echo "- Benchmark tests: $benchmark_tests"
        echo "- Total tests: $((unit_tests + integration_tests + benchmark_tests))"

    } > "$stats_file"

    log_success "Generated project statistics: $stats_file"
}

# Generate API documentation
generate_api_docs() {
    log_section "GENERATING API DOCUMENTATION"

    log_info "Running Doxygen..."

    # Run Doxygen from docs directory (paths in Doxyfile are relative to docs/)
    pushd "$DOCS_DIR" > /dev/null
    if doxygen Doxyfile 2>&1 | tee ../doxygen.log; then
        log_success "Doxygen completed successfully"
    else
        log_error "Doxygen failed"
        log_info "Check doxygen.log for details"
        popd > /dev/null
        exit 1
    fi
    popd > /dev/null

    # Verify output
    if [ -d "$OUTPUT_DIR" ] && [ -f "$OUTPUT_DIR/index.html" ]; then
        local html_files=$(find "$OUTPUT_DIR" -name "*.html" | wc -l)
        log_success "Generated $html_files HTML files"
    else
        log_error "API documentation generation failed"
        exit 1
    fi

    # Clean up log file
    rm -f doxygen.log
}

# Create GitHub Pages configuration
create_github_pages_config() {
    log_section "CREATING GITHUB PAGES CONFIGURATION"

    # Create .nojekyll file to prevent Jekyll processing
    touch "$OUTPUT_DIR/.nojekyll"
    log_success "Created .nojekyll file"

    # Create custom index page if needed
    local custom_index="$OUTPUT_DIR/README.md"
    {
        echo "# Solar System Suite - API Documentation"
        echo ""
        echo "Welcome to the Solar System Suite API documentation."
        echo ""
        echo "## Quick Links"
        echo ""
        echo "- [Main Documentation](index.html)"
        echo "- [Class List](annotated.html)"
        echo "- [File List](files.html)"
        echo "- [Namespace List](namespaces.html)"
        echo ""
        echo "## About"
        echo ""
        echo "The Solar System Suite is a comprehensive N-body gravitational simulation"
        echo "suite with real-time JPL HORIZONS ephemeris data integration and"
        echo "interactive web-based time travel visualization."
        echo ""
        echo "Generated: $(date)"

    } > "$custom_index"

    log_success "Created GitHub Pages configuration"
}

# Validate documentation quality
validate_docs() {
    log_section "VALIDATING DOCUMENTATION QUALITY"

    local issues=0

    # Check for broken links (basic check)
    log_info "Checking for common issues..."

    # Check if main files exist
    local required_files=("index.html" "annotated.html" "files.html")
    for file in "${required_files[@]}"; do
        if [ -f "$OUTPUT_DIR/$file" ]; then
            log_success "✓ $file exists"
        else
            log_warning "✗ $file missing"
            ((issues++))
        fi
    done

    # Check for empty documentation
    local total_classes=$(grep -c "class.*{" lib/**/*.h lib/**/*.hpp 2>/dev/null || echo "0")
    local documented_classes=$(grep -c "class" "$OUTPUT_DIR/annotated.html" 2>/dev/null || echo "0")

    log_info "Classes found in source: $total_classes"
    log_info "Classes in documentation: $documented_classes"

    if [ "$documented_classes" -gt 0 ]; then
        log_success "Documentation contains class information"
    else
        log_warning "No classes found in documentation"
        ((issues++))
    fi

    # Summary
    if [ "$issues" -eq 0 ]; then
        log_success "Documentation validation passed"
    else
        log_warning "Documentation validation found $issues issues"
    fi
}

# Generate deployment summary
generate_deployment_summary() {
    log_section "GENERATING DEPLOYMENT SUMMARY"

    local summary_file="$OUTPUT_DIR/deployment_summary.json"

    {
        echo "{"
        echo "  \"generated\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\","
        echo "  \"version\": \"4.0.0\","
        echo "  \"generator\": \"generate_docs.sh\","
        echo "  \"output_directory\": \"$OUTPUT_DIR\","
        echo "  \"files_generated\": $(find "$OUTPUT_DIR" -name "*.html" | wc -l),"
        echo "  \"total_size_bytes\": $(du -sb "$OUTPUT_DIR" | cut -f1),"
        echo "  \"github_pages_ready\": true,"
        echo "  \"deployment_url\": \"https://your-username.github.io/solarsystem/\""
        echo "}"
    } > "$summary_file"

    log_success "Created deployment summary: $summary_file"
}

# Main execution
main() {
    log_section "SOLAR SYSTEM SUITE - DOCUMENTATION GENERATOR"

    echo "Configuration:"
    echo "  Documentation Directory: $DOCS_DIR"
    echo "  API Output Directory: $OUTPUT_DIR"
    echo "  Doxygen Config: $DOXYGEN_CONFIG"
    echo ""

    # Execute documentation generation pipeline
    verify_prerequisites
    clean_docs
    generate_project_stats
    generate_api_docs
    create_github_pages_config
    validate_docs
    generate_deployment_summary

    # Final summary
    log_section "DOCUMENTATION GENERATION COMPLETE"

    local total_files=$(find "$OUTPUT_DIR" -type f | wc -l)
    local total_size=$(du -sh "$OUTPUT_DIR" | cut -f1)

    log_success "Generated $total_files files ($total_size total)"
    log_info "Documentation available at: $OUTPUT_DIR/index.html"
    log_info "GitHub Pages ready: $OUTPUT_DIR"

    # Instructions for deployment
    echo ""
    echo "GitHub Pages Deployment Instructions:"
    echo "1. Commit the generated documentation:"
    echo "   git add $API_DIR"
    echo "   git commit -m 'Update API documentation'"
    echo ""
    echo "2. Push to GitHub:"
    echo "   git push origin main"
    echo ""
    echo "3. Enable GitHub Pages in repository settings"
    echo "   - Go to Settings > Pages"
    echo "   - Select 'Deploy from a branch'"
    echo "   - Choose 'main' branch and '/$API_DIR/html' folder"

    return 0
}

# Execute main function
main "$@"
