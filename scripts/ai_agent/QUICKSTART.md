# AI Agent Team - Quick Start Guide

## Installation

**Prerequisite**: Python 3.8+ must be installed

```bash
# Check Python version
python --version
```

## Running the Agents

### Option 1: Command Line (Immediate)

```bash
# Navigate to your repository
cd c:/Users/urzam/Documents/GitHub/wireless-pov-poi

# Run code analysis
python scripts/ai_agent/analyze_code.py

# Run error detection
python scripts/ai_agent/detect_errors.py

# Generate new LED patterns
python scripts/ai_agent/generate_patterns.py

# Run the dispatcher (processes tasks in .github/tasks/)
python scripts/ai_agent/agent_dispatcher.py
```

### Option 2: GitHub Actions (Automated)

1. **Push to GitHub**:

   ```bash
   git add .
   git commit -m "Add AI agent team system"
   git push
   ```

2. **Go to Actions Tab**:
   - Visit: <https://github.com/yourusername/wireless-pov-poi/actions>
   - Click "AI Agent Automation"
   - Click "Run workflow"

3. **On-Demand Commands**:
   Comment on any GitHub issue/PR with:
   - `/ai-agent analyze` - Run code analysis
   - `/ai-agent pattern` - Generate patterns
   - `/ai-agent test` - Run tests

## What Each Agent Does

| Agent | Command | What It Does |
| ----- | ------- | ------------ |
| **Analysis** | `analyze_code.py` | Scans all code for errors, warnings, code quality issues |
| **Error Detection** | `detect_errors.py` | Finds bugs, potential failures, security issues |
| **Pattern Generator** | `generate_patterns.py` | Creates new LED patterns (Rainbow, Fire, Plasma, etc.) |
| **Dispatcher** | `agent_dispatcher.py` | Coordinates all agents, processes tasks from `.github/tasks/` |

## Creating New Tasks

1. Create a `.md` file in `.github/tasks/`
2. Add task details with `status: ready`
3. Run dispatcher to process it

Example task file (`.github/tasks/my-task.md`):

```markdown
# Task: My New Task

**Priority**: High
**Type**: analysis

## Description
What the task does

## Requirements
- [ ] Item 1
- [ ] Item 2
```

## Output Files

Results are saved to `scripts/ai_agent/output/`:

- `analyze_code.json` - Analysis results
- `detect_errors.json` - Error detection results
- `generated_patterns/` - New LED pattern files

## Weekly Automation

The GitHub Actions workflow runs automatically:

- **Every Sunday at midnight** - Full analysis
- **On every push/PR** - Quick validation
- **On demand** - Manual trigger or comment commands

## Cost

**$0/month** - Uses GitHub's free tier (2,000 Action minutes/month)

## Troubleshooting

**Python not found?**

```bash
# Ensure Python is in PATH
where python
```

**No tasks found?**

- Check tasks are in `.github/tasks/`
- Files must have `.md` extension
- Must have `status: ready`

**GitHub Actions not running?**

- Check repository Settings → Actions → Enable workflows
- Verify workflow file syntax
