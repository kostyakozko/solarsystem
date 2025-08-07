#!/bin/bash

# Solar System Suite - Git Hooks Setup
# This script installs the automated roadmap update hooks

echo "🔧 Setting up Solar System Suite Git hooks..."

# Create .git/hooks directory if it doesn't exist
mkdir -p .git/hooks

# Copy our hooks
cp .kiro/hooks/pre-commit .git/hooks/pre-commit
cp .kiro/hooks/post-commit .git/hooks/post-commit

# Make them executable
chmod +x .git/hooks/pre-commit
chmod +x .git/hooks/post-commit

echo "✅ Pre-commit hook installed (auto-updates roadmap)"
echo "✅ Post-commit hook installed (shows progress summary)"
echo "📋 Hooks will automatically update roadmap and show progress when task files are modified"

# Test if Python script works
echo "🧪 Testing roadmap update script..."
if python3 .kiro/scripts/update-roadmap.py > /dev/null 2>&1; then
    echo "✅ Roadmap update script works correctly"
else
    echo "❌ Roadmap update script has issues"
    echo "Please check: python3 .kiro/scripts/update-roadmap.py"
    exit 1
fi

echo ""
echo "🎉 Setup complete!"
echo ""
echo "📖 How it works:"
echo "  1. When you commit changes to any tasks.md file"
echo "  2. The hook automatically runs update-roadmap.py"
echo "  3. The updated roadmap is added to your commit"
echo "  4. No manual intervention needed!"
echo ""
echo "🔄 To manually update anytime: python3 .kiro/scripts/update-roadmap.py"
echo "📊 To check status anytime: python3 .kiro/scripts/spec-status.py"
