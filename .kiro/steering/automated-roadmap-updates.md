---
inclusion: always
---

# Automated Roadmap Updates

## Git Hook Integration

When working with Solar System Suite specifications, the following automation is in place:

### Pre-commit Hook
- Automatically detects changes to any `tasks.md` file
- Runs `update-roadmap.py` to refresh the roadmap with current progress
- Adds the updated roadmap to the commit automatically
- Ensures roadmap is always synchronized with task completion status

### Post-commit Hook
- Shows progress summary after commits that modify task files
- Displays current completion statistics
- Provides next recommended actions

## Usage Instructions

### Initial Setup
Run once to install the hooks:
```bash
./.kiro/scripts/setup-hooks.sh
```

### Normal Workflow
1. Mark tasks as completed in any `tasks.md` file (change `[ ]` to `[x]`)
2. Commit your changes normally: `git commit -m "Complete tasks 1-3"`
3. The hooks automatically:
   - Update the roadmap with new progress
   - Add the updated roadmap to your commit
   - Show progress summary after commit

### Manual Commands (if needed)
```bash
# Check current status
python3 .kiro/scripts/spec-status.py

# Manually update roadmap
python3 .kiro/scripts/update-roadmap.py

# Reinstall hooks if needed
./.kiro/scripts/setup-hooks.sh
```

## Benefits

- **Zero maintenance**: Roadmap updates automatically on every commit
- **Always accurate**: Progress reflects actual task completion status
- **No manual steps**: Just mark tasks complete and commit normally
- **Immediate feedback**: See progress summary after each commit
- **Self-documenting**: Progress history preserved in Git commits

This system ensures the roadmap never gets out of sync with actual work completion.
