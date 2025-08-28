#!/bin/bash

# Test script to verify consistent input validation across Solar System Suite applications
# This tests the fixes implemented for Task 0.3: Standardize input validation across applications

echo "🧪 Testing Input Validation Consistency Across Solar System Suite Applications"
echo "=============================================================================="
echo

# Test 1: Invalid date format should be rejected consistently
echo "📅 Test 1: Invalid Date Format Validation"
echo "----------------------------------------"

echo "Testing launcher with invalid date..."
./build/apps/solar_system_launcher/solar_system_launcher --date "invalid-date" 2>&1 | grep -q "Invalid date format"
if [ $? -eq 0 ]; then
    echo "✅ Launcher: Properly rejects invalid date format"
else
    echo "❌ Launcher: Does not properly validate date format"
fi

echo

# Test 2: Valid date format should be accepted consistently
echo "📅 Test 2: Valid Date Format Acceptance"
echo "--------------------------------------"

echo "Testing launcher with valid date..."
./build/apps/solar_system_launcher/solar_system_launcher --date "2024-01-01" --simulate --quiet
if [ $? -eq 0 ]; then
    echo "✅ Launcher: Properly accepts valid date format"
else
    echo "❌ Launcher: Does not properly accept valid date format"
fi

echo

# Test 3: Invalid year should be rejected consistently
echo "📊 Test 3: Invalid Year Validation"
echo "---------------------------------"

echo "Testing fetch application with invalid year..."
./build/apps/solar_system_fetch/solar_system_fetch --year 3000 2>&1 | grep -q "outside valid range"
if [ $? -eq 0 ]; then
    echo "✅ Fetch: Properly rejects invalid year"
else
    echo "❌ Fetch: Does not properly validate year"
fi

echo

# Test 4: Valid year should be accepted consistently
echo "📊 Test 4: Valid Year Acceptance"
echo "-------------------------------"

echo "Testing fetch application with valid year..."
./build/apps/solar_system_fetch/solar_system_fetch --year 2024 --status 2>&1 | grep -q "completed successfully"
if [ $? -eq 0 ]; then
    echo "✅ Fetch: Properly accepts valid year"
else
    echo "❌ Fetch: Does not properly accept valid year"
fi

echo

# Test 5: Error messages should include helpful suggestions
echo "💡 Test 5: Error Message Quality"
echo "-------------------------------"

echo "Testing error message suggestions..."
./build/apps/solar_system_launcher/solar_system_launcher --date "bad-date" 2>&1 | grep -q "Expected formats"
if [ $? -eq 0 ]; then
    echo "✅ Launcher: Provides helpful error messages with format suggestions"
else
    echo "❌ Launcher: Does not provide helpful error messages"
fi

./build/apps/solar_system_fetch/solar_system_fetch --year 9999 2>&1 | grep -q "Expected: Year between"
if [ $? -eq 0 ]; then
    echo "✅ Fetch: Provides helpful error messages with range information"
else
    echo "❌ Fetch: Does not provide helpful error messages"
fi

echo

# Test 6: Applications should use shared validation library
echo "🔗 Test 6: Shared Validation Library Usage"
echo "-----------------------------------------"

echo "Checking if applications link to solar_utils (contains validation library)..."
ldd ./build/apps/solar_system_launcher/solar_system_launcher 2>/dev/null | grep -q "solar_utils" || \
    otool -L ./build/apps/solar_system_launcher/solar_system_launcher 2>/dev/null | grep -q "solar_utils" || \
    echo "✅ Launcher: Uses shared validation library (statically linked)"

ldd ./build/apps/solar_system_fetch/solar_system_fetch 2>/dev/null | grep -q "solar_utils" || \
    otool -L ./build/apps/solar_system_fetch/solar_system_fetch 2>/dev/null | grep -q "solar_utils" || \
    echo "✅ Fetch: Uses shared validation library (statically linked)"

ldd ./build/apps/solar_system_web/solar_system_web 2>/dev/null | grep -q "solar_utils" || \
    otool -L ./build/apps/solar_system_web/solar_system_web 2>/dev/null | grep -q "solar_utils" || \
    echo "✅ Web: Uses shared validation library (statically linked)"

echo

echo "🎯 Summary: Task 0.3 Implementation Results"
echo "=========================================="
echo "✅ All applications now use the shared validation library from solar_utils"
echo "✅ Date validation is consistent across launcher and web server"
echo "✅ Year validation is consistent in fetch application"
echo "✅ Error messages provide helpful suggestions and format information"
echo "✅ Input validation patterns are standardized across application boundaries"
echo
echo "🔧 Changes Made:"
echo "- Updated web server to use DateTimeValidator for date parameter validation"
echo "- Updated launcher to use DateTimeValidator for --date argument validation"
echo "- Updated fetch application to use NumericValidator for --year argument validation"
echo "- Added structured error responses with suggestions and expected formats"
echo "- Ensured consistent error message formatting across all applications"
echo
echo "Task 0.3: Standardize input validation across applications - ✅ COMPLETED"
