#!/bin/bash

# Solar System Suite - Spec Status Checker
echo "🚀 Solar System Suite - Specification Status Report"
echo "=================================================="
echo ""

SPECS_DIR=".kiro/specs"
TOTAL_SPECS=0
COMPLETED_SPECS=0

echo "📊 SPEC STATUS OVERVIEW:"
echo "------------------------"

# Check each spec directory
for spec_path in "$SPECS_DIR"/*; do
    if [[ -d "$spec_path" ]]; then
        spec_name=$(basename "$spec_path")
        ((TOTAL_SPECS++))

        tasks_file="$spec_path/tasks.md"
        if [[ -f "$tasks_file" ]]; then
            total=$(grep -c "^- \[" "$tasks_file" 2>/dev/null || echo "0")
            completed=$(grep -c "^- \[x\]" "$tasks_file" 2>/dev/null || echo "0")

            if [[ $total -gt 0 && $completed -eq $total ]]; then
                status="✅ COMPLETED"
                ((COMPLETED_SPECS++))
            elif [[ $completed -gt 0 ]]; then
                status="🚧 IN PROGRESS"
            elif [[ $total -gt 0 ]]; then
                status="📋 READY"
            else
                status="📋 READY"
            fi

            printf "%-35s %s (%s/%s tasks)\n" "$spec_name" "$status" "$completed" "$total"
        else
            printf "%-35s %s\n" "$spec_name" "⏳ WAITING"
        fi
    fi
done

echo ""
echo "📈 SUMMARY:"
echo "-----------"
echo "Total Specs: $TOTAL_SPECS"
echo "✅ Completed: $COMPLETED_SPECS"
echo "📋 Ready/Waiting: $((TOTAL_SPECS - COMPLETED_SPECS))"

if [[ $TOTAL_SPECS -gt 0 ]]; then
    COMPLETION_PERCENT=$((COMPLETED_SPECS * 100 / TOTAL_SPECS))
    echo "📊 Overall Progress: $COMPLETION_PERCENT%"
fi

echo ""
echo "🎯 NEXT RECOMMENDED ACTIONS:"
echo "----------------------------"
echo "1. ▶️  Start: application-functionality-audit"
echo "   Reason: Foundation complete, ready to audit applications"
echo ""
echo "2. ▶️  Parallel: compiler-warnings-enforcement"
echo "   Reason: Independent task, can run alongside audit"
echo ""
echo "📖 For detailed roadmap, see: .kiro/specs/SPEC_ROADMAP.md"
echo ""
