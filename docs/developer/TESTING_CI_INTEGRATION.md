# Testing Framework CI/CD Integration Guide

## Overview

This guide covers how to integrate the Solar System Testing Framework with various CI/CD systems, including GitHub Actions, Jenkins, GitLab CI, and others. It provides best practices for automated testing, performance monitoring, and deployment validation.

## Table of Contents

1. [GitHub Actions Integration](#github-actions-integration)
2. [Jenkins Integration](#jenkins-integration)
3. [GitLab CI Integration](#gitlab-ci-integration)
4. [Docker Integration](#docker-integration)
5. [Performance Monitoring](#performance-monitoring)
6. [Test Artifacts and Reporting](#test-artifacts-and-reporting)
7. [Best Practices](#best-practices)
8. [Troubleshooting](#troubleshooting)

## GitHub Actions Integration

### Basic Workflow Configuration

```yaml
# .github/workflows/test-suite.yml
name: Solar System Test Suite

on:
  push:
nches: [ main, develop ]
  pull_request:
    branches: [ main ]
  schedule:
    # Run nightly performance tests
    - cron: '0 2 * * *'

env:
  BUILD_TYPE: Release
  ENABLE_TESTING: ON

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
        compiler: [gcc, clang]
        exclude:
          - os: windows-latest
            compiler: gcc
          - os: macos-latest
            compiler: gcc

    steps:
    - name: Checkout Repository
      uses: actions/checkout@v4
      with:
        fetch-depth: 0  # Full history for performance comparison

    - name: Setup Build Environment
      run: |
        if [[ "${{ runner.os }}" == "Linux" ]]; then
          sudo apt-get update
          sudo apt-get install -y build-essential cmake libcurl4-openssl-dev
        elif [[ "${{ runner.os }}" == "macOS" ]]; then
          brew install cmake curl
        fi

    - name: Configure CMake
      run: |
        cmake -B build \
          -DCMAKE_BUILD_TYPE=${{ env.BUILD_TYPE }} \
          -DENABLE_TESTING=${{ env.ENABLE_TESTING }} \
          -DENABLE_COVERAGE=ON

    - name: Build Project
      run: cmake --build build --config ${{ env.BUILD_TYPE }} -j$(nproc)

    - name: Run Unit Tests
      run: |
        cd build
        ctest -L "unit" \
          --output-on-failure \
          --timeout 60 \
          --parallel $(nproc) \
          --output-junit unit-test-results.xml

    - name: Run Integration Tests
      run: |
        cd build
        ctest -L "integration" \
          --output-on-failure \
          --timeout 180 \
          --output-junit integration-test-results.xml

    - name: Run Performance Tests
      if: github.event_name == 'schedule' || contains(github.event.head_commit.message, '[benchmark]')
      run: |
        cd build
        ctest -L "benchmark" \
          --output-on-failure \
          --timeout 300

    - name: Upload Test Results
      if: always()
      uses: actions/upload-artifact@v4
      with:
        name: test-results-${{ matrix.os }}-${{ matrix.compiler }}
        path: |
          build/*-test-results.xml
          build/tests/benchmark_results/

    - name: Publish Test Results
      if: always()
      uses: dorny/test-reporter@v1
      with:
        name: Test Results (${{ matrix.os }}-${{ matrix.compiler }})
        path: 'build/*-test-results.xml'
        reporter: java-junit
```

### Advanced GitHub Actions Features

```yaml
# .github/workflows/advanced-testing.yml
name: Advanced Testing Pipeline

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  code-quality:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4

    - name: Run Static Analysis
      run: |
        sudo apt-get install -y cppcheck clang-tidy

        # Run cppcheck
        cppcheck --enable=all --suppress=missingInclude \
          --xml --xml-version=2 lib/ apps/ 2> cppcheck-report.xml

        # Run clang-tidy
        cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        run-clang-tidy -p build -header-filter='.*' \
          lib/ apps/ > clang-tidy-report.txt

    - name: Check Code Formatting
      run: |
        find lib apps -name "*.cpp" -o -name "*.h" | \
        xargs clang-format --dry-run --Werror

    - name: Upload Analysis Results
      uses: actions/upload-artifact@v4
      with:
        name: code-quality-reports
        path: |
          cppcheck-report.xml
          clang-tidy-report.txt

  security-scan:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4

    - name: Run Security Scan
      uses: github/codeql-action/init@v2
      with:
        languages: cpp

    - name: Build for Security Analysis
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Debug
        cmake --build build

    - name: Perform CodeQL Analysis
      uses: github/codeql-action/analyze@v2

  performance-regression:
    runs-on: ubuntu-latest
    if: github.event_name == 'pull_request'
    steps:
    - uses: actions/checkout@v4
      with:
        fetch-depth: 0

    - name: Build and Run Benchmarks
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON
        cmake --build build -j$(nproc)
        cd build && ctest -L "benchmark" --output-on-failure

    - name: Download Baseline Performance Data
      uses: actions/download-artifact@v4
      with:
        name: performance-baseline
        path: baseline/
      continue-on-error: true

    - name: Compare Performance
      run: |
        python3 tests/scripts/compare_performance.py \
          --baseline baseline/comprehensive_baseline.csv \
          --current build/tests/benchmark_results/ \
          --threshold 10 \
          --output performance-comparison.json

    - name: Comment Performance Results
      uses: actions/github-script@v6
      with:
        script: |
          const fs = require('fs');
          const path = 'performance-comparison.json';

          if (fs.existsSync(path)) {
            const results = JSON.parse(fs.readFileSync(path, 'utf8'));

            let comment = '## Performance Comparison Results\n\n';

            if (results.regressions.length > 0) {
              comment += '### ⚠️ Performance Regressions Detected\n\n';
              results.regressions.forEach(reg => {
                comment += `- **${reg.benchmark}**: ${reg.degradation}% slower\n`;
              });
            }

            if (results.improvements.length > 0) {
              comment += '### ✅ Performance Improvements\n\n';
              results.improvements.forEach(imp => {
                comment += `- **${imp.benchmark}**: ${imp.improvement}% faster\n`;
              });
            }

            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: comment
            });
          }

  memory-leak-detection:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4

    - name: Install Valgrind
      run: sudo apt-get install -y valgrind

    - name: Build with Debug Symbols
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
        cmake --build build

    - name: Run Tests with Valgrind
      run: |
        cd build
        ctest -L "unit" -T memcheck --output-on-failure

    - name: Upload Memory Check Results
      if: always()
      uses: actions/upload-artifact@v4
      with:
        name: memory-check-results
        path: build/Testing/Temporary/MemoryChecker.*.log
```

## Jenkins Integration

### Declarative Pipeline

```groovy
// Jenkinsfile
pipeline {
    agent {
        docker {
            image 'ubuntu:22.04'
            args '-v /var/run/docker.sock:/var/run/docker.sock'
        }
    }

    parameters {
        choice(
            name: 'BUILD_TYPE',
            choices: ['Debug', 'Release', 'RelWithDebInfo'],
            description: 'CMake build type'
        )
        choice(
            name: 'TEST_SUITE',
            choices: ['all', 'unit', 'integration', 'performance'],
            description: 'Test suite to run'
        )
        booleanParam(
            name: 'RUN_BENCHMARKS',
            defaultValue: false,
            description: 'Run performance benchmarks'
        )
        booleanParam(
            name: 'GENERATE_COVERAGE',
            defaultValue: false,
            description: 'Generate code coverage report'
        )
    }

    environment {
        CC = 'gcc-10'
        CXX = 'g++-10'
        MAKEFLAGS = '-j4'
    }

    stages {
        stage('Setup Environment') {
            steps {
                sh '''
                    apt-get update
                    apt-get install -y \
                        build-essential \
                        cmake \
                        libcurl4-openssl-dev \
                        python3 \
                        python3-pip \
                        lcov \
                        valgrind

                    pip3 install pandas matplotlib
                '''
            }
        }

        stage('Build') {
            steps {
                sh '''
                    cmake -B build \
                        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
                        -DENABLE_TESTING=ON \
                        -DENABLE_COVERAGE=${GENERATE_COVERAGE}

                    cmake --build build --config ${BUILD_TYPE} ${MAKEFLAGS}
                '''
            }
        }

        stage('Test') {
            parallel {
                stage('Unit Tests') {
                    when {
                        anyOf {
                            params.TEST_SUITE == 'all'
                            params.TEST_SUITE == 'unit'
                        }
                    }
                    steps {
                        sh '''
                            cd build
                            ctest -L "unit" \
                                --output-on-failure \
                                --output-junit unit-test-results.xml \
                                --timeout 60
                        '''
                    }
                    post {
                        always {
                            publishTestResults testResultsPattern: 'build/unit-test-results.xml'
                        }
                    }
                }

                stage('Integration Tests') {
                    when {
                        anyOf {
                            params.TEST_SUITE == 'all'
                            params.TEST_SUITE == 'integration'
                        }
                    }
                    steps {
                        sh '''
                            cd build
                            ctest -L "integration" \
                                --output-on-failure \
                                --output-junit integration-test-results.xml \
                                --timeout 300
                        '''
                    }
                    post {
                        always {
                            publishTestResults testResultsPattern: 'build/integration-test-results.xml'
                        }
                    }
                }

                stage('Performance Tests') {
                    when {
                        anyOf {
                            params.TEST_SUITE == 'all'
                            params.TEST_SUITE == 'performance'
                            params.RUN_BENCHMARKS == true
                        }
                    }
                    steps {
                        sh '''
                            cd build
                            ctest -L "benchmark" \
                                --output-on-failure \
                                --timeout 600

                            # Generate performance report
                            python3 ../tests/scripts/generate_performance_report.py \
                                --input tests/benchmark_results/ \
                                --output performance-report.html
                        '''
                    }
                    post {
                        always {
                            publishHTML([
                                allowMissing: false,
                                alwaysLinkToLastBuild: true,
                                keepAll: true,
                                reportDir: 'build',
                                reportFiles: 'performance-report.html',
                                reportName: 'Performance Report'
                            ])
                            archiveArtifacts artifacts: 'build/tests/benchmark_results/**/*'
                        }
                    }
                }
            }
        }

        stage('Code Coverage') {
            when {
                params.GENERATE_COVERAGE == true
            }
            steps {
                sh '''
                    cd build

                    # Generate coverage data
                    lcov --capture --directory . --output-file coverage.info
                    lcov --remove coverage.info '/usr/*' --output-file coverage.info
                    lcov --remove coverage.info '*/tests/*' --output-file coverage.info

                    # Generate HTML report
                    genhtml coverage.info --output-directory coverage-report

                    # Calculate coverage percentage
                    COVERAGE=$(lcov --summary coverage.info | grep lines | cut -d' ' -f4)
                    echo "Coverage: $COVERAGE"

                    # Fail if coverage is below threshold
                    COVERAGE_NUM=$(echo $COVERAGE | sed 's/%//')
                    if (( $(echo "$COVERAGE_NUM < 80" | bc -l) )); then
                        echo "Coverage $COVERAGE is below 80% threshold"
                        exit 1
                    fi
                '''
            }
            post {
                always {
                    publishHTML([
                        allowMissing: false,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'build/coverage-report',
                        reportFiles: 'index.html',
                        reportName: 'Coverage Report'
                    ])
                }
            }
        }

        stage('Memory Leak Detection') {
            when {
                params.BUILD_TYPE == 'Debug'
            }
            steps {
                sh '''
                    cd build
                    ctest -L "unit" -T memcheck --output-on-failure
                '''
            }
            post {
                always {
                    archiveArtifacts artifacts: 'build/Testing/Temporary/MemoryChecker.*.log'
                }
            }
        }
    }

    post {
        always {
            // Archive build artifacts
            archiveArtifacts artifacts: 'build/bin/*', allowEmptyArchive: true

            // Clean workspace
            cleanWs()
        }

        success {
            // Update performance baseline on main branch
            script {
                if (env.BRANCH_NAME == 'main' && params.RUN_BENCHMARKS) {
                    sh '''
                        cp build/tests/benchmark_results/comprehensive_benchmark.csv \
                           baseline_performance/comprehensive_baseline.csv

                        git add baseline_performance/comprehensive_baseline.csv
                        git commit -m "Update performance baseline [skip ci]"
                        git push origin main
                    '''
                }
            }
        }

        failure {
            emailext (
                subject: "Build Failed: ${env.JOB_NAME} - ${env.BUILD_NUMBER}",
                body: """
                Build failed for ${env.JOB_NAME} - ${env.BUILD_NUMBER}

                Build URL: ${env.BUILD_URL}
                Branch: ${env.BRANCH_NAME}
                Commit: ${env.GIT_COMMIT}

                Check the console output for details.
                """,
                to: "${env.CHANGE_AUTHOR_EMAIL}",
                attachLog: true
            )
        }
    }
}
```

## GitLab CI Integration

```yaml
# .gitlab-ci.yml
stages:
  - build
  - test
  - performance
  - deploy

variables:
  CMAKE_BUILD_TYPE: "Release"
  ENABLE_TESTING: "ON"

# Build stage
build:
  stage: build
  image: ubuntu:22.04
  before_script:
    - apt-get update -qq
    - apt-get install -y build-essential cmake libcurl4-openssl-dev
  script:
    - cmake -B build -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DENABLE_TESTING=$ENABLE_TESTING
    - cmake --build build -j$(nproc)
  artifacts:
    paths:
      - build/
    expire_in: 1 hour

# Unit tests
unit-tests:
  stage: test
  image: ubuntu:22.04
  dependencies:
    - build
  script:
    - cd build
    - ctest -L "unit" --output-on-failure --output-junit unit-test-results.xml
  artifacts:
    reports:
      junit: build/unit-test-results.xml
    paths:
      - build/unit-test-results.xml
    expire_in: 1 week

# Integration tests
integration-tests:
  stage: test
  image: ubuntu:22.04
  dependencies:
    - build
  script:
    - cd build
    - ctest -L "integration" --output-on-failure --output-junit integration-test-results.xml
  artifacts:
    reports:
      junit: build/integration-test-results.xml
    paths:
      - build/integration-test-results.xml
    expire_in: 1 week

# Performance tests
performance-tests:
  stage: performance
  image: ubuntu:22.04
  dependencies:
    - build
  script:
    - cd build
    - ctest -L "benchmark" --output-on-failure --timeout 600
    - python3 ../tests/scripts/generate_performance_report.py
  artifacts:
    paths:
      - build/tests/benchmark_results/
      - build/performance-report.html
    expire_in: 1 month
  only:
    - main
    - develop
    - /^performance\/.*$/

# Code coverage
coverage:
  stage: test
  image: ubuntu:22.04
  before_script:
    - apt-get update -qq
    - apt-get install -y build-essential cmake libcurl4-openssl-dev lcov
  script:
    - cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON -DENABLE_COVERAGE=ON
    - cmake --build build -j$(nproc)
    - cd build
    - ctest -L "unit;integration" --output-on-failure
    - lcov --capture --directory . --output-file coverage.info
    - lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
    - genhtml coverage.info --output-directory coverage-report
  coverage: '/lines......: \d+\.\d+%/'
  artifacts:
    paths:
      - build/coverage-report/
    reports:
      coverage_report:
        coverage_format: cobertura
        path: build/coverage.xml
    expire_in: 1 month

# Docker build and test
docker-test:
  stage: test
  image: docker:latest
  services:
    - docker:dind
  script:
    - docker build -t solar-system-test .
    - docker run --rm solar-system-test ctest --output-on-failure
  only:
    - main
    - merge_requests

# Security scanning
security-scan:
  stage: test
  image: ubuntu:22.04
  before_script:
    - apt-get update -qq
    - apt-get install -y cppcheck
  script:
    - cppcheck --enable=all --xml --xml-version=2 lib/ apps/ 2> cppcheck-report.xml
  artifacts:
    paths:
      - cppcheck-report.xml
    expire_in: 1 week

# Deploy documentation
pages:
  stage: deploy
  dependencies:
    - coverage
    - performance-tests
  script:
    - mkdir public
    - cp -r build/coverage-report/* public/
    - cp build/performance-report.html public/
  artifacts:
    paths:
      - public
  only:
    - main
```

## Docker Integration

### Multi-stage Dockerfile for Testing

```dockerfile
# Dockerfile.test
FROM ubuntu:22.04 AS base

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    python3 \
    python3-pip \
    lcov \
    valgrind \
    cppcheck \
    clang-tidy \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages for analysis
RUN pip3 install pandas matplotlib numpy

WORKDIR /app

# Copy source code
COPY . .

# Build stage
FROM base AS build
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON
RUN cmake --build build -j$(nproc)

# Test stage
FROM build AS test
RUN cd build && ctest --output-on-failure

# Performance test stage
FROM build AS performance
RUN cd build && ctest -L "benchmark" --output-on-failure --timeout 600

# Coverage stage
FROM base AS coverage
RUN cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON -DENABLE_COVERAGE=ON
RUN cmake --build build -j$(nproc)
RUN cd build && ctest -L "unit;integration" --output-on-failure
RUN cd build && lcov --capture --directory . --output-file coverage.info
RUN cd build && lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
RUN cd build && genhtml coverage.info --output-directory coverage-report

# Production stage
FROM ubuntu:22.04 AS production
RUN apt-get update && apt-get install -y libcurl4 && rm -rf /var/lib/apt/lists/*
COPY --from=build /app/build/bin/* /usr/local/bin/
COPY --from=build /app/build/lib/* /usr/local/lib/
COPY --from=build /app/install/ /usr/local/
CMD ["solar_system_launcher", "--help"]
```

### Docker Compose for Testing

```yaml
# docker-compose.test.yml
version: '3.8'

services:
  test-runner:
    build:
      context: .
      dockerfile: Dockerfile.test
      target: test
    volumes:
      - ./test-results:/app/test-results
    environment:
      - CTEST_OUTPUT_ON_FAILURE=1
    command: >
      bash -c "
        cd build &&
        ctest --output-junit /app/test-results/test-results.xml
      "

  performance-runner:
    build:
      context: .
      dockerfile: Dockerfile.test
      target: performance
    volumes:
      - ./performance-results:/app/performance-results
    command: >
      bash -c "
        cd build &&
        ctest -L benchmark --output-on-failure &&
        cp -r tests/benchmark_results/* /app/performance-results/
      "

  coverage-runner:
    build:
      context: .
      dockerfile: Dockerfile.test
      target: coverage
    volumes:
      - ./coverage-results:/app/coverage-results
    command: >
      bash -c "
        cd build &&
        cp -r coverage-report/* /app/coverage-results/
      "

  static-analysis:
    build:
      context: .
      dockerfile: Dockerfile.test
      target: base
    volumes:
      - ./analysis-results:/app/analysis-results
    command: >
      bash -c "
        cppcheck --enable=all --xml --xml-version=2 lib/ apps/ 2> /app/analysis-results/cppcheck-report.xml &&
        clang-tidy -p build lib/**/*.cpp apps/**/*.cpp > /app/analysis-results/clang-tidy-report.txt
      "
```

## Performance Monitoring

### Automated Performance Tracking

```python
# tests/scripts/performance_tracker.py
#!/usr/bin/env python3

import json
import csv
import os
import sys
from datetime import datetime
import argparse

class PerformanceTracker:
    def __init__(self, baseline_file, results_dir):
        self.baseline_file = baseline_file
        self.results_dir = results_dir
        self.baseline_data = self.load_baseline()

    def load_baseline(self):
        """Load baseline performance data"""
        if os.path.exists(self.baseline_file):
            with open(self.baseline_file, 'r') as f:
                reader = csv.DictReader(f)
                return {row['Name']: row for row in reader}
        return {}

    def collect_current_results(self):
        """Collect current benchmark results"""
        results = {}
        for file in os.listdir(self.results_dir):
            if file.endswith('.csv'):
                file_path = os.path.join(self.results_dir, file)
                with open(file_path, 'r') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        results[row['Name']] = row
        return results

    def detect_regressions(self, threshold=10.0):
        """Detect performance regressions"""
        current_results = self.collect_current_results()
        regressions = []
        improvements = []

        for benchmark_name, current_data in current_results.items():
            if benchmark_name in self.baseline_data:
                baseline_time = float(self.baseline_data[benchmark_name]['AvgDuration(ms)'])
                current_time = float(current_data['AvgDuration(ms)'])

                change_percent = ((current_time - baseline_time) / baseline_time) * 100

                if change_percent > threshold:
                    regressions.append({
                        'benchmark': benchmark_name,
                        'baseline_time': baseline_time,
                        'current_time': current_time,
                        'degradation_percent': change_percent
                    })
                elif change_percent < -5.0:  # Improvement threshold
                    improvements.append({
                        'benchmark': benchmark_name,
                        'baseline_time': baseline_time,
                        'current_time': current_time,
                        'improvement_percent': abs(change_percent)
                    })

        return regressions, improvements

    def generate_report(self, output_file):
        """Generate performance report"""
        regressions, improvements = self.detect_regressions()

        report = {
            'timestamp': datetime.now().isoformat(),
            'summary': {
                'total_benchmarks': len(self.collect_current_results()),
                'regressions_count': len(regressions),
                'improvements_count': len(improvements)
            },
            'regressions': regressions,
            'improvements': improvements
        }

        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)

        return report

    def update_baseline(self, benchmark_name=None):
        """Update baseline with current results"""
        current_results = self.collect_current_results()

        if benchmark_name:
            if benchmark_name in current_results:
                self.baseline_data[benchmark_name] = current_results[benchmark_name]
        else:
            self.baseline_data.update(current_results)

        # Write updated baseline
        with open(self.baseline_file, 'w', newline='') as f:
            if self.baseline_data:
                fieldnames = list(next(iter(self.baseline_data.values())).keys())
                writer = csv.DictWriter(f, fieldnames=fieldnames)
                writer.writeheader()
                for data in self.baseline_data.values():
                    writer.writerow(data)

def main():
    parser = argparse.ArgumentParser(description='Track performance regressions')
    parser.add_argument('--baseline', required=True, help='Baseline performance file')
    parser.add_argument('--results', required=True, help='Current results directory')
    parser.add_argument('--threshold', type=float, default=10.0, help='Regression threshold (%)')
    parser.add_argument('--output', required=True, help='Output report file')
    parser.add_argument('--update-baseline', action='store_true', help='Update baseline with current results')

    args = parser.parse_args()

    tracker = PerformanceTracker(args.baseline, args.results)
    report = tracker.generate_report(args.output)

    # Print summary
    print(f"Performance Analysis Summary:")
    print(f"  Total benchmarks: {report['summary']['total_benchmarks']}")
    print(f"  Regressions: {report['summary']['regressions_count']}")
    print(f"  Improvements: {report['summary']['improvements_count']}")

    if report['regressions']:
        print("\nRegressions detected:")
        for reg in report['regressions']:
            print(f"  - {reg['benchmark']}: {reg['degradation_percent']:.1f}% slower")

    if report['improvements']:
        print("\nImprovements detected:")
        for imp in report['improvements']:
            print(f"  - {imp['benchmark']}: {imp['improvement_percent']:.1f}% faster")

    if args.update_baseline:
        tracker.update_baseline()
        print("\nBaseline updated with current results")

    # Exit with error code if regressions found
    if report['regressions']:
        sys.exit(1)

if __name__ == '__main__':
    main()
```

## Test Artifacts and Reporting

### Comprehensive Test Report Generation

```python
# tests/scripts/generate_test_report.py
#!/usr/bin/env python3

import xml.etree.ElementTree as ET
import json
import csv
import os
import sys
from datetime import datetime
import argparse

class TestReportGenerator:
    def __init__(self):
        self.test_results = {}
        self.performance_data = {}
        self.coverage_data = {}

    def parse_junit_xml(self, xml_file):
        """Parse JUnit XML test results"""
        tree = ET.parse(xml_file)
        root = tree.getroot()

        suite_name = root.get('name', 'Unknown')
        tests = int(root.get('tests', 0))
        failures = int(root.get('failures', 0))
        errors = int(root.get('errors', 0))
        time = float(root.get('time', 0))

        test_cases = []
        for testcase in root.findall('testcase'):
            case = {
                'name': testcase.get('name'),
                'classname': testcase.get('classname'),
                'time': float(testcase.get('time', 0)),
                'status': 'passed'
            }

            if testcase.find('failure') is not None:
                case['status'] = 'failed'
                case['failure_message'] = testcase.find('failure').text
            elif testcase.find('error') is not None:
                case['status'] = 'error'
                case['error_message'] = testcase.find('error').text
            elif testcase.find('skipped') is not None:
                case['status'] = 'skipped'

            test_cases.append(case)

        return {
            'suite_name': suite_name,
            'summary': {
                'total': tests,
                'passed': tests - failures - errors,
                'failed': failures,
                'errors': errors,
                'execution_time': time
            },
            'test_cases': test_cases
        }

    def parse_performance_csv(self, csv_file):
        """Parse performance benchmark CSV"""
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            return list(reader)

    def parse_coverage_info(self, coverage_file):
        """Parse LCOV coverage info"""
        coverage_data = {
            'lines_total': 0,
            'lines_covered': 0,
            'functions_total': 0,
            'functions_covered': 0,
            'branches_total': 0,
            'branches_covered': 0
        }

        if os.path.exists(coverage_file):
            with open(coverage_file, 'r') as f:
                for line in f:
                    if line.startswith('LF:'):
                        coverage_data['lines_total'] = int(line.split(':')[1])
                    elif line.startswith('LH:'):
                        coverage_data['lines_covered'] = int(line.split(':')[1])
                    elif line.startswith('FNF:'):
                        coverage_data['functions_total'] = int(line.split(':')[1])
                    elif line.startswith('FNH:'):
                        coverage_data['functions_covered'] = int(line.split(':')[1])
                    elif line.startswith('BRF:'):
                        coverage_data['branches_total'] = int(line.split(':')[1])
                    elif line.startswith('BRH:'):
                        coverage_data['branches_covered'] = int(line.split(':')[1])

        # Calculate percentages
        if coverage_data['lines_total'] > 0:
            coverage_data['line_coverage_percent'] = (
                coverage_data['lines_covered'] / coverage_data['lines_total'] * 100
            )

        if coverage_data['functions_total'] > 0:
            coverage_data['function_coverage_percent'] = (
                coverage_data['functions_covered'] / coverage_data['functions_total'] * 100
            )

        if coverage_data['branches_total'] > 0:
            coverage_data['branch_coverage_percent'] = (
                coverage_data['branches_covered'] / coverage_data['branches_total'] * 100
            )

        return coverage_data

    def generate_html_report(self, output_file):
        """Generate comprehensive HTML report"""
        html_template = """
<!DOCTYPE html>
<html>
<head>
    <title>Solar System Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .summary { display: flex; gap: 20px; margin: 20px 0; }
        .summary-card { background-color: #f9f9f9; padding: 15px; border-radius: 5px; flex: 1; }
        .passed { color: green; }
        .failed { color: red; }
        .error { color: orange; }
        .skipped { color: gray; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .performance-chart { margin: 20px 0; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Solar System Test Report</h1>
        <p>Generated: {timestamp}</p>
    </div>

    <div class="summary">
        <div class="summary-card">
            <h3>Test Results</h3>
            <p>Total Tests: {total_tests}</p>
            <p class="passed">Passed: {passed_tests}</p>
            <p class="failed">Failed: {failed_tests}</p>
            <p class="error">Errors: {error_tests}</p>
            <p class="skipped">Skipped: {skipped_tests}</p>
        </div>

        <div class="summary-card">
            <h3>Code Coverage</h3>
            <p>Line Coverage: {line_coverage:.1f}%</p>
            <p>Function Coverage: {function_coverage:.1f}%</p>
            <p>Branch Coverage: {branch_coverage:.1f}%</p>
        </div>

        <div class="summary-card">
            <h3>Performance</h3>
            <p>Benchmarks Run: {benchmark_count}</p>
            <p>Average Execution Time: {avg_execution_time:.2f}ms</p>
            <p>Memory Usage: {memory_usage:.1f}MB</p>
        </div>
    </div>

    <h2>Test Results by Suite</h2>
    {test_results_table}

    <h2>Performance Benchmarks</h2>
    {performance_table}

    <h2>Coverage Details</h2>
    {coverage_details}

</body>
</html>
        """

        # Calculate summary statistics
        total_tests = sum(suite['summary']['total'] for suite in self.test_results.values())
        passed_tests = sum(suite['summary']['passed'] for suite in self.test_results.values())
        failed_tests = sum(suite['summary']['failed'] for suite in self.test_results.values())
        error_tests = sum(suite['summary']['errors'] for suite in self.test_results.values())
        skipped_tests = total_tests - passed_tests - failed_tests - error_tests

        # Generate HTML content
        html_content = html_template.format(
            timestamp=datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            total_tests=total_tests,
            passed_tests=passed_tests,
            failed_tests=failed_tests,
            error_tests=error_tests,
            skipped_tests=skipped_tests,
            line_coverage=self.coverage_data.get('line_coverage_percent', 0),
            function_coverage=self.coverage_data.get('function_coverage_percent', 0),
            branch_coverage=self.coverage_data.get('branch_coverage_percent', 0),
            benchmark_count=len(self.performance_data),
            avg_execution_time=sum(float(b.get('AvgDuration(ms)', 0)) for b in self.performance_data) / max(len(self.performance_data), 1),
            memory_usage=sum(float(b.get('MemoryUsage(bytes)', 0)) for b in self.performance_data) / (1024 * 1024 * max(len(self.performance_data), 1)),
            test_results_table=self._generate_test_results_table(),
            performance_table=self._generate_performance_table(),
            coverage_details=self._generate_coverage_details()
        )

        with open(output_file, 'w') as f:
            f.write(html_content)

    def _generate_test_results_table(self):
        """Generate HTML table for test results"""
        if not self.test_results:
            return "<p>No test results available</p>"

        html = "<table><tr><th>Suite</th><th>Total</th><th>Passed</th><th>Failed</th><th>Errors</th><th>Time (s)</th></tr>"

        for suite_name, suite_data in self.test_results.items():
            summary = suite_data['summary']
            html += f"""
            <tr>
                <td>{suite_name}</td>
                <td>{summary['total']}</td>
                <td class="passed">{summary['passed']}</td>
                <td class="failed">{summary['failed']}</td>
                <td class="error">{summary['errors']}</td>
                <td>{summary['execution_time']:.2f}</td>
            </tr>
            """

        html += "</table>"
        return html

    def _generate_performance_table(self):
        """Generate HTML table for performance results"""
        if not self.performance_data:
            return "<p>No performance data available</p>"

        html = "<table><tr><th>Benchmark</th><th>Avg Time (ms)</th><th>Min Time (ms)</th><th>Max Time (ms)</th><th>Ops/Sec</th><th>Memory (MB)</th></tr>"

        for benchmark in self.performance_data:
            memory_mb = float(benchmark.get('MemoryUsage(bytes)', 0)) / (1024 * 1024)
            html += f"""
            <tr>
                <td>{benchmark.get('Name', 'Unknown')}</td>
                <td>{benchmark.get('AvgDuration(ms)', 'N/A')}</td>
                <td>{benchmark.get('MinDuration(ms)', 'N/A')}</td>
                <td>{benchmark.get('MaxDuration(ms)', 'N/A')}</td>
                <td>{benchmark.get('OpsPerSec', 'N/A')}</td>
                <td>{memory_mb:.2f}</td>
            </tr>
            """

        html += "</table>"
        return html

    def _generate_coverage_details(self):
        """Generate HTML for coverage details"""
        if not self.coverage_data:
            return "<p>No coverage data available</p>"

        return f"""
        <table>
            <tr><th>Metric</th><th>Covered</th><th>Total</th><th>Percentage</th></tr>
            <tr>
                <td>Lines</td>
                <td>{self.coverage_data.get('lines_covered', 0)}</td>
                <td>{self.coverage_data.get('lines_total', 0)}</td>
                <td>{self.coverage_data.get('line_coverage_percent', 0):.1f}%</td>
            </tr>
            <tr>
                <td>Functions</td>
                <td>{self.coverage_data.get('functions_covered', 0)}</td>
                <td>{self.coverage_data.get('functions_total', 0)}</td>
                <td>{self.coverage_data.get('function_coverage_percent', 0):.1f}%</td>
            </tr>
            <tr>
                <td>Branches</td>
                <td>{self.coverage_data.get('branches_covered', 0)}</td>
                <td>{self.coverage_data.get('branches_total', 0)}</td>
                <td>{self.coverage_data.get('branch_coverage_percent', 0):.1f}%</td>
            </tr>
        </table>
        """

def main():
    parser = argparse.ArgumentParser(description='Generate comprehensive test report')
    parser.add_argument('--junit-xml', nargs='+', help='JUnit XML files')
    parser.add_argument('--performance-csv', nargs='+', help='Performance CSV files')
    parser.add_argument('--coverage-info', help='LCOV coverage info file')
    parser.add_argument('--output', required=True, help='Output HTML file')

    args = parser.parse_args()

    generator = TestReportGenerator()

    # Parse test results
    if args.junit_xml:
        for xml_file in args.junit_xml:
            if os.path.exists(xml_file):
                result = generator.parse_junit_xml(xml_file)
                generator.test_results[result['suite_name']] = result

    # Parse performance data
    if args.performance_csv:
        for csv_file in args.performance_csv:
            if os.path.exists(csv_file):
                data = generator.parse_performance_csv(csv_file)
                generator.performance_data.extend(data)

    # Parse coverage data
    if args.coverage_info and os.path.exists(args.coverage_info):
        generator.coverage_data = generator.parse_coverage_info(args.coverage_info)

    # Generate report
    generator.generate_html_report(args.output)
    print(f"Test report generated: {args.output}")

if __name__ == '__main__':
    main()
```

## Best Practices

### 1. Test Organization
- Separate unit, integration, and performance tests
- Use consistent naming conventions
- Tag tests appropriately for filtering
- Maintain test data in version control

### 2. CI/CD Pipeline Design
- Fail fast with unit tests
- Run integration tests in parallel when possible
- Schedule performance tests during off-peak hours
- Use caching to speed up builds

### 3. Performance Monitoring
- Establish baseline performance metrics
- Set reasonable regression thresholds (typically 10-15%)
- Track performance trends over time
- Alert on significant regressions

### 4. Resource Management
- Clean up test artifacts after runs
- Use temporary directories for test isolation
- Monitor memory usage in CI environments
- Set appropriate timeouts for different test types

### 5. Reporting and Notifications
- Generate comprehensive test reports
- Notify relevant team members of failures
- Archive test artifacts for debugging
- Provide actionable error messages

## Troubleshooting

### Common CI/CD Issues

#### Test Discovery Problems
```bash
# Check if tests are properly registered
ctest --show-only

# Verify test labels
ctest --print-labels

# Run specific test with verbose output
ctest -R "TestName" --verbose
```

#### Performance Test Instability
```bash
# Run performance tests multiple times
for i in {1..5}; do
  ctest -L "benchmark" --output-on-failure
done

# Use statistical analysis for stability
python3 tests/scripts/analyze_performance_stability.py
```

#### Memory Issues in CI
```bash
# Monitor memory usage during tests
/usr/bin/time -v ctest --output-on-failure

# Use memory-efficient test execution
ctest --parallel 1  # Disable parallel execution
```

#### Docker Build Issues
```bash
# Build with verbose output
docker build --progress=plain -t solar-system-test .

# Run container interactively for debugging
docker run -it --entrypoint /bin/bash solar-system-test
```

This comprehensive CI/CD integration guide provides the foundation for robust automated testing and continuous validation of the Solar System Suite project.
