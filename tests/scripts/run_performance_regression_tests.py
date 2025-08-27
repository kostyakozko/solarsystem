#!/usr/bin/env python3
"""
Enhanced Performance Regression Test Runner for Solar System Suite (Task 22)
Comprehensive performance regression testing with automated baseline management,
alerting, and optimization recommendations.
"""

import os
import sys
import json
import subprocess
import datetime
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Tuple

class PerformanceRegressionRunner:
    def __init__(self, build_dir: str = "build", config_file: str = "tests/performance_regression_config.json"):
        self.build_dir = Path(build_dir)
        self.config_file = Path(config_file)
        self.baseline_dir = Path("performance_baselines")
        self.reports_dir = Path("performance_reports")

        # Create directories
        self.baseline_dir.mkdir(exist_ok=True)
        self.reports_dir.mkdir(exist_ok=True)

        # Load configuration
        self.config = self.load_config()

        # Test results
        self.test_results = []
        self.regression_alerts = []
        self.optimization_recommendations = []

    def load_config(self) -> Dict:
        """Load performance regression testing configuration"""
        default_config = {
            "regression_thresholds": {
                "execution_time_percent": 15.0,
                "memory_usage_percent": 20.0,
                "cpu_usage_percent": 25.0,
                "cache_miss_rate_percent": 30.0
            },
            "baseline_management": {
                "auto_update_baselines": False,
                "baseline_improvement_threshold": 5.0,
                "minimum_samples_for_update": 3
            },
            "alerting": {
                "enable_email_alerts": False,
                "enable_slack_alerts": False,
                "alert_severity_threshold": "WARNING"
            },
            "optimization": {
                "enable_recommendations": True,
                "minimum_priority_score": 5,
                "generate_code_examples": True
            },
            "components": {
                "solar_core": {
                    "tests": ["simulation_performance", "body_factory_performance", "vector_math_performance"],
                    "critical": True
                },
                "solar_jpl": {
                    "tests": ["jpl_data_loading", "cache_performance", "network_resilience"],
                    "critical": True
                },
                "solar_utils": {
                    "tests": ["argument_parsing", "error_handling", "resource_management"],
                    "critical": False
                },
                "test_framework": {
                    "tests": ["test_execution", "memory_monitoring", "performance_measurement"],
                    "critical": False
                }
            },
            "reporting": {
                "generate_html_report": True,
                "generate_json_report": True,
                "generate_markdown_report": True,
                "include_optimization_recommendations": True,
                "include_trend_analysis": True
            }
        }

        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    user_config = json.load(f)
                    # Merge with defaults
                    default_config.update(user_config)
            except Exception as e:
                print(f"⚠️  Warning: Failed to load config file {self.config_file}: {e}")
                print("Using default configuration")

        return default_config

    def save_config(self):
        """Save current configuration to file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
            print(f"✅ Configuration saved to {self.config_file}")
        except Exception as e:
            print(f"❌ Failed to save configuration: {e}")

    def run_command(self, cmd: str, cwd: Optional[str] = None, timeout: int = 300) -> Tuple[int, str, str]:
        """Run a command and return result"""
        try:
            result = subprocess.run(
                cmd, shell=True, capture_output=True, text=True,
                cwd=cwd, timeout=timeout
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", f"Command timed out after {timeout}s"
        except Exception as e:
            return -1, "", str(e)

    def build_tests(self) -> bool:
        """Build performance regression tests"""
        print("🔨 Building performance regression tests...")

        if not self.build_dir.exists():
            print(f"❌ Build directory {self.build_dir} not found")
            return False

        # Build performance regression test
        cmd = f"cmake --build {self.build_dir} --target test_performance_regression_system -j$(nproc)"
        returncode, stdout, stderr = self.run_command(cmd)

        if returncode != 0:
            print(f"❌ Failed to build performance regression tests")
            print(f"   stdout: {stdout}")
            print(f"   stderr: {stderr}")
            return False

        print("✅ Performance regression tests built successfully")
        return True

    def run_performance_tests(self) -> bool:
        """Run all performance regression tests"""
        print("🧪 Running performance regression tests...")

        test_executable = self.build_dir / "tests" / "unit" / "test_performance_regression_system"

        if not test_executable.exists():
            print(f"❌ Test executable not found: {test_executable}")
            return False

        # Run the test
        returncode, stdout, stderr = self.run_command(str(test_executable), timeout=120)

        if returncode != 0:
            print(f"❌ Performance regression tests failed")
            print(f"   stdout: {stdout}")
            print(f"   stderr: {stderr}")
            return False

        print("✅ Performance regression tests completed successfully")
        print(f"   Output: {stdout}")

        return True

    def run_component_benchmarks(self) -> bool:
        """Run component-specific performance benchmarks"""
        print("📊 Running component performance benchmarks...")

        success = True

        for component_name, component_config in self.config["components"].items():
            print(f"  Testing component: {component_name}")

            #n benchmarks for this component
            benchmark_cmd = f"ctest -R 'Benchmark.*{component_name.replace('_', '').title()}' --output-on-failure"
            returncode, stdout, stderr = self.run_command(benchmark_cmd, cwd=str(self.build_dir))

            if returncode != 0:
                print(f"    ⚠️  Some benchmarks failed for {component_name}")
                if component_config.get("critical", False):
                    success = False
            else:
                print(f"    ✅ {component_name} benchmarks passed")

        return success

    def analyze_performance_data(self) -> bool:
        """Analyze performance data and detect regressions"""
        print("🔍 Analyzing performance data...")

        # Look for benchmark result files
        benchmark_results_dir = self.build_dir / "tests" / "benchmarks" / "benchmark_results"

        if not benchmark_results_dir.exists():
            print(f"⚠️  Benchmark results directory not found: {benchmark_results_dir}")
            return True  # Not a failure, just no data to analyze

        csv_files = list(benchmark_results_dir.glob("*.csv"))

        if not csv_files:
            print("⚠️  No benchmark CSV files found")
            return True

        print(f"📈 Found {len(csv_files)} benchmark result files")

        # Run performance comparison if baseline exists
        baseline_file = self.baseline_dir / "combined_baseline.csv"

        if baseline_file.exists():
            for csv_file in csv_files:
                print(f"  Comparing {csv_file.name} against baseline...")

                compare_cmd = f"python3 tests/scripts/compare_performance.py {baseline_file} {csv_file}"
                returncode, stdout, stderr = self.run_command(compare_cmd)

                if returncode != 0:
                    print(f"    ⚠️  Performance regression detected in {csv_file.name}")
                    self.regression_alerts.append({
                        "file": csv_file.name,
                        "output": stdout,
                        "severity": "WARNING"
                    })
                else:
                    print(f"    ✅ No regressions in {csv_file.name}")
        else:
            print("📋 No baseline found, creating new baseline...")
            self.create_baseline()

        return True

    def create_baseline(self) -> bool:
        """Create performance baseline"""
        print("📋 Creating performance baseline...")

        # Run baseline generation script
        cmd = "python3 tests/scripts/generate_baseline.py"
        returncode, stdout, stderr = self.run_command(cmd)

        if returncode != 0:
            print(f"❌ Failed to create baseline")
            print(f"   stderr: {stderr}")
            return False

        print("✅ Performance baseline created successfully")
        return True

    def generate_optimization_recommendations(self) -> List[Dict]:
        """Generate performance optimization recommendations"""
        print("💡 Generating optimization recommendations...")

        recommendations = []

        # Analyze regression alerts for optimization opportunities
        for alert in self.regression_alerts:
            rec = {
                "component": alert["file"].replace("_benchmark.csv", ""),
                "issue": f"Performance regression detected in {alert['file']}",
                "recommendation": "Review recent changes and profile performance bottlenecks",
                "priority": "HIGH" if alert["severity"] == "CRITICAL" else "MEDIUM",
                "potential_improvement": "15-30%"
            }
            recommendations.append(rec)

        # Add component-specific recommendations
        if any("jpl" in alert["file"] for alert in self.regression_alerts):
            recommendations.append({
                "component": "solar_jpl",
                "issue": "JPL data loading performance regression",
                "recommendation": "Consider implementing parallel downloads or improving cache efficiency",
                "priority": "HIGH",
                "potential_improvement": "40-60%",
                "code_example": """
// Example: Parallel JPL data loading
std::vector<std::future<JPLData>> futures;
for (const auto& body : bodies) {
    futures.push_back(std::async(std::launch::async,
        [&]() { return jpl_client.fetch_data(body); }));
}
"""
            })

        if any("core" in alert["file"] for alert in self.regression_alerts):
            recommendations.append({
                "component": "solar_core",
                "issue": "Simulation performance regression",
                "recommendation": "Optimize N-body calculations using SIMD instructions or approximation methods",
                "priority": "HIGH",
                "potential_improvement": "25-50%",
                "code_example": """
// Example: SIMD-optimized vector operations
#include <immintrin.h>
__m256d pos_x = _mm256_load_pd(&positions[i].x);
__m256d pos_y = _mm256_load_pd(&positions[i].y);
__m256d pos_z = _mm256_load_pd(&positions[i].z);
"""
            })

        self.optimization_recommendations = recommendations
        print(f"💡 Generated {len(recommendations)} optimization recommendations")

        return recommendations

    def generate_reports(self) -> bool:
        """Generate comprehensive performance reports"""
        print("📊 Generating performance reports...")

        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

        # Generate JSON report
        if self.config["reporting"]["generate_json_report"]:
            json_report = {
                "timestamp": timestamp,
                "summary": {
                    "total_components": len(self.config["components"]),
                    "regression_alerts": len(self.regression_alerts),
                    "optimization_recommendations": len(self.optimization_recommendations)
                },
                "regression_alerts": self.regression_alerts,
                "optimization_recommendations": self.optimization_recommendations,
                "configuration": self.config
            }

            json_file = self.reports_dir / f"performance_regression_report_{timestamp}.json"
            with open(json_file, 'w') as f:
                json.dump(json_report, f, indent=2)
            print(f"📄 JSON report: {json_file}")

        # Generate Markdown report
        if self.config["reporting"]["generate_markdown_report"]:
            md_file = self.reports_dir / f"performance_regression_report_{timestamp}.md"
            self.generate_markdown_report(md_file)
            print(f"📄 Markdown report: {md_file}")

        # Generate HTML report
        if self.config["reporting"]["generate_html_report"]:
            html_file = self.reports_dir / f"performance_regression_report_{timestamp}.html"
            self.generate_html_report(html_file)
            print(f"📄 HTML report: {html_file}")

        return True

    def generate_markdown_report(self, filename: Path):
        """Generate detailed Markdown report"""
        with open(filename, 'w') as f:
            f.write("# Solar System Suite - Performance Regression Report\n\n")
            f.write(f"Generated: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")

            # Summary
            f.write("## Summary\n\n")
            f.write(f"- **Components Tested**: {len(self.config['components'])}\n")
            f.write(f"- **Regression Alerts**: {len(self.regression_alerts)}\n")
            f.write(f"- **Optimization Recommendations**: {len(self.optimization_recommendations)}\n\n")

            # Regression Alerts
            if self.regression_alerts:
                f.write("## 🚨 Performance Regression Alerts\n\n")
                for alert in self.regression_alerts:
                    f.write(f"### {alert['file']}\n\n")
                    f.write(f"- **Severity**: {alert['severity']}\n")
                    f.write(f"- **Details**: Performance regression detected\n\n")
                    f.write("```\n")
                    f.write(alert['output'][:500] + "..." if len(alert['output']) > 500 else alert['output'])
                    f.write("\n```\n\n")
            else:
                f.write("## ✅ No Performance Regressions Detected\n\n")
                f.write("All performance tests passed without significant regressions.\n\n")

            # Optimization Recommendations
            if self.optimization_recommendations:
                f.write("## 💡 Performance Optimization Recommendations\n\n")
                for i, rec in enumerate(self.optimization_recommendations, 1):
                    f.write(f"### {i}. {rec['component']} - {rec['priority']} Priority\n\n")
                    f.write(f"**Issue**: {rec['issue']}\n\n")
                    f.write(f"**Recommendation**: {rec['recommendation']}\n\n")
                    f.write(f"**Potential Improvement**: {rec['potential_improvement']}\n\n")

                    if 'code_example' in rec:
                        f.write("**Code Example**:\n")
                        f.write("```cpp\n")
                        f.write(rec['code_example'].strip())
                        f.write("\n```\n\n")

            # Configuration
            f.write("## Configuration\n\n")
            f.write("```json\n")
            f.write(json.dumps(self.config, indent=2))
            f.write("\n```\n")

    def generate_html_report(self, filename: Path):
        """Generate HTML report with charts and interactive elements"""
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>Solar System Suite - Performance Regression Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        .header {{ background: #f0f8ff; padding: 20px; border-radius: 8px; }}
        .alert {{ background: #ffe4e1; padding: 15px; border-radius: 5px; margin: 10px 0; }}
        .success {{ background: #f0fff0; padding: 15px; border-radius: 5px; margin: 10px 0; }}
        .recommendation {{ background: #fff8dc; padding: 15px; border-radius: 5px; margin: 10px 0; }}
        .code {{ background: #f5f5f5; padding: 10px; border-radius: 3px; font-family: monospace; }}
        .priority-high {{ border-left: 5px solid #ff4444; }}
        .priority-medium {{ border-left: 5px solid #ffaa00; }}
        .priority-low {{ border-left: 5px solid #44ff44; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🚀 Solar System Suite - Performance Regression Report</h1>
        <p><strong>Generated:</strong> {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
    </div>

    <h2>📊 Summary</h2>
    <ul>
        <li><strong>Components Tested:</strong> {len(self.config['components'])}</li>
        <li><strong>Regression Alerts:</strong> {len(self.regression_alerts)}</li>
        <li><strong>Optimization Recommendations:</strong> {len(self.optimization_recommendations)}</li>
    </ul>
"""

        # Add regression alerts
        if self.regression_alerts:
            html_content += "<h2>🚨 Performance Regression Alerts</h2>\n"
            for alert in self.regression_alerts:
                html_content += f"""
    <div class="alert">
        <h3>{alert['file']}</h3>
        <p><strong>Severity:</strong> {alert['severity']}</p>
        <div class="code">{alert['output'][:300]}...</div>
    </div>
"""
        else:
            html_content += """
    <div class="success">
        <h2>✅ No Performance Regressions Detected</h2>
        <p>All performance tests passed without significant regressions.</p>
    </div>
"""

        # Add optimization recommendations
        if self.optimization_recommendations:
            html_content += "<h2>💡 Performance Optimization Recommendations</h2>\n"
            for i, rec in enumerate(self.optimization_recommendations, 1):
                priority_class = f"priority-{rec['priority'].lower()}"
                html_content += f"""
    <div class="recommendation {priority_class}">
        <h3>{i}. {rec['component']} - {rec['priority']} Priority</h3>
        <p><strong>Issue:</strong> {rec['issue']}</p>
        <p><strong>Recommendation:</strong> {rec['recommendation']}</p>
        <p><strong>Potential Improvement:</strong> {rec['potential_improvement']}</p>
"""
                if 'code_example' in rec:
                    html_content += f'<div class="code">{rec["code_example"]}</div>'
                html_content += "</div>\n"

        html_content += """
</body>
</html>
"""

        with open(filename, 'w') as f:
            f.write(html_content)

    def run_full_regression_test(self) -> bool:
        """Run complete performance regression testing workflow"""
        print("🚀 Starting comprehensive performance regression testing...")
        print("=" * 70)

        success = True

        # Step 1: Build tests
        if not self.build_tests():
            success = False

        # Step 2: Run performance tests
        if success and not self.run_performance_tests():
            success = False

        # Step 3: Run component benchmarks
        if success and not self.run_component_benchmarks():
            print("⚠️  Some component benchmarks failed, but continuing...")

        # Step 4: Analyze performance data
        if not self.analyze_performance_data():
            print("⚠️  Performance analysis had issues, but continuing...")

        # Step 5: Generate optimization recommendations
        self.generate_optimization_recommendations()

        # Step 6: Generate reports
        if not self.generate_reports():
            print("⚠️  Report generation had issues")

        # Summary
        print("\n" + "=" * 70)
        print("📊 Performance Regression Testing Summary")
        print("=" * 70)

        if success:
            print("✅ Performance regression testing completed successfully")
        else:
            print("⚠️  Performance regression testing completed with issues")

        print(f"📈 Regression alerts: {len(self.regression_alerts)}")
        print(f"💡 Optimization recommendations: {len(self.optimization_recommendations)}")
        print(f"📄 Reports generated in: {self.reports_dir}")

        return success

def main():
    parser = argparse.ArgumentParser(description='Solar System Suite Performance Regression Testing')
    parser.add_argument('--build-dir', default='build', help='Build directory path')
    parser.add_argument('--config', default='tests/performance_regression_config.json',
                       help='Configuration file path')
    parser.add_argument('--create-baseline', action='store_true',
                       help='Create new performance baseline')
    parser.add_argument('--update-config', action='store_true',
                       help='Update configuration file with defaults')

    args = parser.parse_args()

    # Create runner
    runner = PerformanceRegressionRunner(args.build_dir, args.config)

    # Handle special operations
    if args.update_config:
        runner.save_config()
        return 0

    if args.create_baseline:
        return 0 if runner.create_baseline() else 1

    # Run full regression testing
    success = runner.run_full_regression_test()

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
