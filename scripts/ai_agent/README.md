# AI Agent Team System for Wireless POV POI

Free-tier autonomous AI agent system for repository analysis, error detection, and automated development.

## Overview

This system provides a multi-agent architecture that can:
- Analyze code for errors and potential failures
- Detect bugs and security issues
- Generate new LED patterns
- Run automated tests
- Coordinate via GitHub Actions

## Cost: $0/month (Free Tier)

| Tool | Cost | Purpose |
|------|------|---------|
| GitHub Actions | 2,000 mins/mo free | CI/CD automation |
| Python | Free | Script execution |
| GitHub Issues | Free | Task management |
| **Total** | **$0** | Fully autonomous |

## Quick Start

### 1. Run Analysis

```bash
# Full repository analysis
python scripts/ai_agent/analyze_code.py '{"name":"my_analysis"}'

# Error detection
python scripts/ai_agent/detect_errors.py '{"name":"error_check"}'

# Generate patterns
python scripts/ai_agent/generate_patterns.py '{"num_patterns":4}'
```

### 2. Run Dispatcher (Process Tasks)

```bash
python scripts/ai_agent/agent_dispatcher.py
```

### 3. GitHub Actions

The workflow runs automatically on:
- Weekly schedule (Sunday midnight)
- Pull requests
- Manual trigger
- Comment `/ai-agent analyze|pattern|test`

## Agent Scripts

### analyze_code.py
Comprehensive code analysis for errors, quality, and potential failures.

**Usage:**
```bash
python scripts/ai_agent/analyze_code.py '{"name":"task_name"}'
```

**Output:** `scripts/ai_agent/output/analyze_code.json`

### detect_errors.py
Detects bugs, potential failures, and security issues.

**Usage:**
```bash
python scripts/ai_agent/detect_errors.py '{"name":"task_name"}'
```

**Output:** `scripts/ai_agent/output/detect_errors.json`

### generate_patterns.py
Generates new LED patterns for the POV display.

**Usage:**
```bash
# Generate random patterns
python scripts/ai_agent/generate_patterns.py '{"num_patterns":4}'

# Generate specific patterns
python scripts/ai_agent/generate_patterns.py '{"pattern_ids":["rainbow_cycle","fire_effect"]}'
```

**Output:** `scripts/ai_agent/output/generated_patterns/`

### agent_dispatcher.py
Coordinates all agents and processes tasks from `.github/tasks/`

**Usage:**
```bash
python scripts/ai_agent/agent_dispatcher.py
```

## Task System

Tasks are defined as markdown files in `.github/tasks/`:

```markdown
# Task: Example Task

**Priority**: High
**Type**: analysis

## Description
Task description here

## Requirements
- [ ] Requirement 1
- [ ] Requirement 2

## Acceptance Criteria
- [ ] Criterion 1
```

### Available Task Templates

- `analysis-full-repository.md` - Complete repository analysis
- `firmware-integration-testing.md` - PlatformIO firmware testing
- `generate-new-patterns.md` - Create new LED patterns

## GitHub Actions Workflow

The workflow (`.github/workflows/ai-agent-automation.yml`) provides:

1. **AI Code Analysis** - Runs on push/PR/weekly
2. **Error Detection** - Validates code quality
3. **Pattern Generation** - Creates new patterns
4. **Testing** - Runs pytest tests
5. **Firmware Build** - Verifies compilation

## Project-Specific Analysis

### Firmware Files Checked
- `firmware/teensy41/**/*.cpp` - PlatformIO Teensy firmware
- `esp32_firmware/**/*.ino` - ESP32 firmware
- `teensy_firmware/**/*.ino` - Arduino Teensy firmware

### Patterns Generated
- Rainbow Cycle
- Sine Wave
- Comet Tail
- Fire Effect
- Plasma Wave
- Pulse Beat
- Noise Shimmer
- Spiral Trace

## Output Files

Results are saved to `scripts/ai_agent/output/`:

```
output/
├── analyze_code.json      # Code analysis results
├── detect_errors.json     # Error detection results
├── dispatcher_summary.json # Task dispatch summary
├── dispatcher.log         # Dispatcher log
└── generated_patterns/    # New LED patterns
```

## Adding New Tasks

1. Create `.github/tasks/your-task-name.md`
2. Add task details (see template above)
3. Mark with `status: ready`
4. Run dispatcher: `python scripts/ai_agent/agent_dispatcher.py`

## On-Demand Execution

Comment on GitHub issues with:

```
/ai-agent analyze   # Run code analysis
/ai-agent pattern   # Generate patterns
/ai-agent test      # Run tests
```

## Troubleshooting

### Python Not Found
```bash
# Ensure Python 3.10+ is installed
python --version
```

### No Tasks Found
- Check tasks are in `.github/tasks/`
- Ensure files have `.md` extension
- Mark with `status: ready`

### GitHub Actions Not Running
- Check workflow file syntax
- Verify repository permissions
- Check Actions tab for errors

## Success Metrics

| Metric | Target |
|--------|--------|
| Code Quality Score | >80 |
| Test Pass Rate | >95% |
| Critical Errors | 0 |
| Patterns Generated | 4+/month |

## License

MIT License - Free for any use.

## Support

- Create GitHub issue for bugs
- Check `plans/AI_AGENT_TEAM_SETUP.md` for architecture details
- Review `REMAINING_WORK.md` for project status
