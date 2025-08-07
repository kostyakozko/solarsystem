#!/usr/bin/env python3

import os
import re
from pathlib import Path
from datetime import datetime

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
    elif completed == total and total > 0:
        return "✅ COMPLETED"
    elif completed > 0:
        return "🚧 IN PROGRESS"
    else:
        return "📋 READY"

def analyze_specs():
    """Analyze all specs and return status information"""
    specs_dir = Path(".kiro/specs")
    specs_info = {}

    for spec_path in sorted(specs_dir.iterdir()):
        if spec_path.is_dir() and not spec_path.name.startswith('.'):
            tasks_file = spec_path / "tasks.md"
            completed, total = count_tasks(tasks_file)
            status = get_spec_status(completed, total)

            specs_info[spec_path.name] = {
                'completed': completed,
                'total': total,
                'status': status,
                'status_emoji': status.split()[0]  # Just the emoji
            }

    return specs_info

def update_roadmap():
    """Update the SPEC_ROADMAP.md file with current status"""
    specs_info = analyze_specs()

    # Count totals
    total_specs = len(specs_info)
    completed_specs = sum(1 for spec in specs_info.values() if spec['status'] == '✅ COMPLETED')
    in_progress_specs = sum(1 for spec in specs_info.values() if spec['status'] == '🚧 IN PROGRESS')
    ready_specs = total_specs - completed_specs - in_progress_specs

    completion_percent = (completed_specs * 100) // total_specs if total_specs > 0 else 0

    # Generate the updated roadmap content
    roadmap_content = f"""# Solar System Suite - Specification Roadmap & Status Tracker

## 📊 Overall Progress Dashboard

| Status | Count | Percentage |
|--------|-------|------------|
| ✅ **Completed** | {completed_specs} | {completion_percent}% |
| 🚧 **In Progress** | {in_progress_specs} | {(in_progress_specs * 100) // total_specs if total_specs > 0 else 0}% |
| 📋 **Ready to Start** | {ready_specs} | {(ready_specs * 100) // total_specs if total_specs > 0 else 0}% |
| **Total Specs** | **{total_specs}** | **100%** |

---

## 🎯 Recommended Implementation Order

### **Phase 1: Foundation & Stability** (Immediate Priority)
*Focus: Get the core system stable and fully functional*

#### 1. ✅ **library-application-fixes** - COMPLETED
- **Status**: ✅ All 8 tasks completed
- **Achievement**: Integration tests now 100% passing
- **Impact**: Critical foundation for all other work

#### 2. ✅ **testing-framework-enhancement** - COMPLETED
- **Status**: ✅ All 14 tasks completed
- **Achievement**: Test framework is fully functional
- **Impact**: Enables reliable testing for all future development

#### 3. 📋 **application-functionality-audit** - READY TO START
- **Status**: 📋 Ready ({specs_info.get('application-functionality-audit', {}).get('completed', 0)}/{specs_info.get('application-functionality-audit', {}).get('total', 0)} tasks completed)
- **Priority**: 🔥 **HIGH** - Next recommended spec
- **Dependencies**: None (can start immediately)
- **Estimated Duration**: 1-2 weeks
- **Why Next**: Validates current application functionality before enhancements

#### 4. 📋 **library-core-enhancements** - READY TO START
- **Status**: 📋 Ready ({specs_info.get('library-core-enhancements', {}).get('completed', 0)}/{specs_info.get('library-core-enhancements', {}).get('total', 0)} tasks completed)
- **Priority**: 🔥 **HIGH**
- **Dependencies**: Should follow application-functionality-audit
- **Estimated Duration**: 2-3 weeks
- **Why Important**: Completes placeholder implementations and core functionality

### **Phase 2: Feature Enhancement** (Short-term)
*Focus: Add missing functionality and improve user experience*

#### 5. 📋 **application-enhancements** - READY TO START
- **Status**: 📋 Ready ({specs_info.get('application-enhancements', {}).get('completed', 0)}/{specs_info.get('application-enhancements', {}).get('total', 0)} tasks completed)
- **Priority**: 🔥 **HIGH**
- **Dependencies**: Requires library-core-enhancements completion
- **Estimated Duration**: 3-4 weeks
- **Why Important**: Adds missing CLI options (--bodies, --duration, etc.) that tests expect

#### 6. 📋 **compiler-warnings-enforcement** - READY TO START
- **Status**: 📋 Ready ({specs_info.get('compiler-warnings-enforcement', {}).get('completed', 0)}/{specs_info.get('compiler-warnings-enforcement', {}).get('total', 0)} tasks completed)
- **Priority**: 🟡 **MEDIUM**
- **Dependencies**: None (can run in parallel)
- **Estimated Duration**: 1 week
- **Why Important**: Improves code quality and prevents regressions

### **Phase 3: Quality & Testing** (Medium-term)
*Focus: Comprehensive testing and quality assurance*

#### 7. ⏳ **test-suite-completion** - WAITING
- **Status**: ⏳ Blocked ({specs_info.get('test-suite-completion', {}).get('completed', 0)}/{specs_info.get('test-suite-completion', {}).get('total', 0)} tasks completed)
- **Priority**: 🟡 **MEDIUM**
- **Dependencies**: Requires Phase 1 & 2 completion
- **Estimated Duration**: 4-6 weeks
- **Why Waiting**: Needs stable foundation before comprehensive testing

### **Phase 4: Advanced Features** (Long-term)
*Focus: Advanced capabilities and specialized tools*

#### 8. ⏳ **performance-monitoring-system** - WAITING
- **Status**: ⏳ Blocked ({specs_info.get('performance-monitoring-system', {}).get('completed', 0)}/{specs_info.get('performance-monitoring-system', {}).get('total', 0)} tasks completed)
- **Priority**: 🟢 **LOW**
- **Dependencies**: Requires stable core system
- **Estimated Duration**: 3-4 weeks

#### 9. ⏳ **data-analysis-tools** - WAITING
- **Status**: ⏳ Blocked ({specs_info.get('data-analysis-tools', {}).get('completed', 0)}/{specs_info.get('data-analysis-tools', {}).get('total', 0)} tasks completed)
- **Priority**: 🟢 **LOW**
- **Dependencies**: Requires core functionality completion
- **Estimated Duration**: 4-6 weeks

#### 10. ⏳ **local-ci-testing** - WAITING
- **Status**: ⏳ Blocked ({specs_info.get('local-ci-testing', {}).get('completed', 0)}/{specs_info.get('local-ci-testing', {}).get('total', 0)} tasks completed)
- **Priority**: 🟢 **LOW**
- **Dependencies**: Requires test suite completion
- **Estimated Duration**: 2-3 weeks

### **Phase 5: Advanced Integration** (Future)
*Focus: Advanced collaboration and comprehensive planning*

#### 11. ⏳ **collaborative-validation-framework** - WAITING
- **Status**: ⏳ Blocked ({specs_info.get('collaborative-validation-framework', {}).get('completed', 0)}/{specs_info.get('collaborative-validation-framework', {}).get('total', 0)} tasks completed)
- **Priority**: 🟢 **LOW**
- **Dependencies**: Requires most other specs completion
- **Estimated Duration**: 3-4 weeks

#### 12. ⏳ **comprehensive-implementation-roadmap** - WAITING
- **Status**: ⏳ Blocked ({specs_info.get('comprehensive-implementation-roadmap', {}).get('completed', 0)}/{specs_info.get('comprehensive-implementation-roadmap', {}).get('total', 0)} tasks completed)
- **Priority**: 🟢 **LOW**
- **Dependencies**: Should be done after major functionality is complete
- **Estimated Duration**: 2-3 weeks

---

## 📋 Detailed Spec Status

### ✅ **COMPLETED SPECS**

#### library-application-fixes
- **Completion**: 8/8 tasks (100%)
- **Key Achievements**:
  - Fixed CelestialBody availability logic
  - Resolved web server startup issues
  - Enhanced application command-line interfaces
  - Stabilized test environment
  - Achieved cross-platform compatibility
  - Validated CI/CD pipeline success
  - Addressed library and application issues
- **Impact**: Integration tests now 100% passing

#### testing-framework-enhancement
- **Completion**: 14/14 tasks (100%)
- **Key Achievements**:
  - Complete test framework implementation
  - Comprehensive testing utilities
  - Test data management system
  - Performance measurement capabilities
  - Mock and stub systems
- **Impact**: Reliable testing foundation for all development

### 📋 **READY TO START** (Can begin immediately)

#### application-functionality-audit
- **Tasks**: {specs_info.get('application-functionality-audit', {}).get('completed', 0)}/{specs_info.get('application-functionality-audit', {}).get('total', 0)} completed
- **Focus**: Audit all applications for basic functionality
- **Key Areas**: Launcher, fetch, simulation, realtime, web applications
- **Why Ready**: No dependencies, builds on completed foundation

#### library-core-enhancements
- **Tasks**: {specs_info.get('library-core-enhancements', {}).get('completed', 0)}/{specs_info.get('library-core-enhancements', {}).get('total', 0)} completed
- **Focus**: Complete placeholder implementations in core libraries
- **Key Areas**: JPL client, body factory, simulation builder, test framework
- **Why Ready**: Foundation is stable, can enhance core functionality

#### application-enhancements
- **Tasks**: {specs_info.get('application-enhancements', {}).get('completed', 0)}/{specs_info.get('application-enhancements', {}).get('total', 0)} completed
- **Focus**: Add missing application features and options
- **Key Areas**: CLI options (--bodies, --duration, etc.), configuration management
- **Why Ready**: Applications are functional, ready for feature additions

#### compiler-warnings-enforcement
- **Tasks**: {specs_info.get('compiler-warnings-enforcement', {}).get('completed', 0)}/{specs_info.get('compiler-warnings-enforcement', {}).get('total', 0)} completed
- **Focus**: Enforce consistent compiler warnings across build system
- **Key Areas**: CMake configuration, warning flags, CI integration
- **Why Ready**: Independent of other specs, can run in parallel

### ⏳ **BLOCKED/WAITING** (Dependencies not met)

#### test-suite-completion
- **Tasks**: {specs_info.get('test-suite-completion', {}).get('completed', 0)}/{specs_info.get('test-suite-completion', {}).get('total', 0)} completed
- **Blocking Factor**: Needs stable core functionality before comprehensive testing
- **Dependencies**: Phases 1-2 completion recommended

#### Other waiting specs
- All other specs are blocked pending completion of foundation and core functionality

---

## 🎯 **IMMEDIATE NEXT STEPS**

### **Recommended Action Plan:**

1. **Start application-functionality-audit** (Next Week)
   - Begin with Task 1: Audit solar_system_launcher
   - Focus on validating current functionality
   - Document any issues found

2. **Parallel: Begin compiler-warnings-enforcement** (This Week)
   - Independent task, can run alongside audit
   - Quick win for code quality

3. **Plan library-core-enhancements** (Following Week)
   - Review placeholder implementations
   - Prioritize most critical completions

### **Success Metrics:**
- Application audit reveals functionality gaps
- Compiler warnings are consistently enforced
- Core library placeholders are identified and prioritized
- Clear path to Phase 2 established

---

## 📈 **Progress Tracking**

### **Weekly Review Questions:**
1. Which tasks were completed this week?
2. What blockers were encountered?
3. Are we on track for the current phase?
4. Should priorities be adjusted?

### **Phase Completion Criteria:**
- **Phase 1**: All applications audited, core libraries enhanced
- **Phase 2**: Missing features added, warnings enforced
- **Phase 3**: Comprehensive test coverage achieved
- **Phase 4**: Advanced features operational
- **Phase 5**: Full integration and documentation complete

### **Risk Factors:**
- 🔴 **High Risk**: Attempting Phase 3+ before Phase 1-2 completion
- 🟡 **Medium Risk**: Parallel development without coordination
- 🟢 **Low Risk**: Following recommended sequence

---

*Last Updated: {datetime.now().strftime('%Y-%m-%d')}*
*Next Review: Weekly*
*Auto-generated by update-roadmap.py*"""

    # Write the updated roadmap
    roadmap_path = Path(".kiro/specs/SPEC_ROADMAP.md")
    roadmap_path.write_text(roadmap_content)

    print(f"✅ Updated {roadmap_path}")
    print(f"📊 Status: {completed_specs}/{total_specs} specs completed ({completion_percent}%)")

    return specs_info

def main():
    print("🔄 Updating SPEC_ROADMAP.md with current status...")
    specs_info = update_roadmap()

    print("\n📋 Current Status:")
    for spec_name, info in specs_info.items():
        if info['status'] == '✅ COMPLETED':
            print(f"  ✅ {spec_name} - {info['completed']}/{info['total']} tasks")

if __name__ == "__main__":
    main()
