#!/usr/bin/env python3

import os
import re
from pathlib import Path

def count_tasks(tasks_file):
    """Count total and completed tasks in a tasks.md file"""
    if not tasks_file.exists():
        return 0, 0

    content = tasks_file.read_text()
    total = len(re.findall(r'^- \[', content, re.MULTILINE))
    completed = len(re.findall(r'^- \[x\]', content, re.MULTILINE))
    return completed, total

def get_spec_status(completed, total):
    """Determine spec status based on task completion"""
    if total == 0:
        return "📋 READY"
    elif completed == total:
        return "✅ COMPLETED"
    elif completed > 0:
        return "🚧 IN PROGRESS"
    else:
        return "📋 READY"

def main():
    print("🚀 Solar System Suite - Specification Status Report")
    print("=" * 50)
    print()

    specs_dir = Path(".kiro/specs")
    if not specs_dir.exists():
        print("❌ Specs directory not found!")
        return

    specs = []
    total_specs = 0
    completed_specs = 0

    print("📊 SPEC STATUS OVERVIEW:")
    print("-" * 24)

    for spec_path in sorted(specs_dir.iterdir()):
        if spec_path.is_dir() and not spec_path.name.startswith('.'):
            total_specs += 1
            tasks_file = spec_path / "tasks.md"
            completed, total = count_tasks(tasks_file)
            status = get_spec_status(completed, total)

            if status == "✅ COMPLETED":
                completed_specs += 1

            print(f"{spec_path.name:<35} {status} ({completed}/{total} tasks)")
            specs.append((spec_path.name, status, completed, total))

    print()
    print("📈 SUMMARY:")
    print("-" * 11)
    print(f"Total Specs: {total_specs}")
    print(f"✅ Completed: {completed_specs}")
    print(f"📋 Ready/Waiting: {total_specs - completed_specs}")

    if total_specs > 0:
        completion_percent = (completed_specs * 100) // total_specs
        print(f"📊 Overall Progress: {completion_percent}%")

    print()
    print("🎯 NEXT RECOMMENDED ACTIONS:")
    print("-" * 28)
    print("1. ▶️  Start: application-functionality-audit")
    print("   Reason: Foundation complete, ready to audit applications")
    print()
    print("2. ▶️  Parallel: compiler-warnings-enforcement")
    print("   Reason: Independent task, can run alongside audit")
    print()
    print("📖 For detailed roadmap, see: .kiro/specs/SPEC_ROADMAP.md")
    print("🔄 To update roadmap: python3 .kiro/scripts/update-roadmap.py")
    print()

if __name__ == "__main__":
    main()
