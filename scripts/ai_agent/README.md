# AI Agent Team System for Wireless POV POI

Real, working AI agent system for automated repository analysis, error detection, and pattern generation. No placeholders - all outputs are actual working code and data.

## Overview

This system provides autonomous agents that:
- Analyze code with real static analysis + compileall + pytest
- Detect bugs and errors through multiple checks
- Generate actual LED pattern files (both .h headers and .json data)
- Run via GitHub Actions on push, PR, schedule, or manual trigger
- Coordinate tasks from `.github/tasks/*.md` files

## Cost: $0/month (Free Tier)

| Tool | Cost | Purpose |
|------|------|---------|
| GitHub Actions | 2,000 mins/mo free | CI/CD automation |
| Python | Free | Script execution |
| GitHub Issues | Free | Task management |
| **Total** | **$0** | Fully autonomous |

## Quick Start

### 1. Install Dependencies

```bash
pip install -r scripts/ai_agent/requirements.txt
# Or skip PlatformIO: SKIP_PIO=1 pip install -r scripts/ai_agent/requirements.txt
```

### 2. Run Individual Scripts

```bash
# Full code analysis (Python compileall, pytest, optional PlatformIO)
python scripts/ai_agent/analyze_code.py '{"name":"my_analysis"}'
# Output: scripts/ai_agent/output/analyze_code.json

# Error detection (pytest, py_compile, TODO finder)
python scripts/ai_agent/detect_errors.py '{"name":"error_check"}'
# Output: scripts/ai_agent/output/detect_errors.json

# Generate LED patterns (creates real .h and .json files)
python scripts/ai_agent/generate_patterns.py '{"num_patterns":4}'
# Output: scripts/ai_agent/output/generated_patterns/pattern_*.{h,json}
```

### 3. Run Dispatcher (Process All Tasks)

```bash
# Reads .github/tasks/*.md, runs appropriate scripts based on Type field
python scripts/ai_agent/agent_dispatcher.py
# Output: scripts/ai_agent/output/dispatcher_summary.json + dispatcher.log
```

### 4. GitHub Actions

The workflow runs automatically on:
- **Push to main/master** - Runs analysis and error detection
- **Pull request** - Runs analysis and error detection  
- **Weekly schedule** - Sunday midnight UTC, runs full dispatcher
- **Manual trigger** - workflow_dispatch in Actions tab
- **Issue comment** - `/ai-agent analyze|pattern|test` in any issue comment

## Agent Scripts

### analyze_code.py

Comprehensive code analysis with real checks:
- **Python compileall**: `python -m compileall .` on entire repo
- **Pytest**: Runs all tests in `examples/` directory
- **PlatformIO build**: Optional firmware compilation (skip with `SKIP_PIO=1`)
- **Static analysis**: Pattern matching for common issues
- **Metrics**: Code quality score, file counts, issue severity

**Usage:**
```bash
python scripts/ai_agent/analyze_code.py '{"name":"task_name"}'
# Set SKIP_PIO=1 to skip PlatformIO build
```

**Output:** `scripts/ai_agent/output/analyze_code.json` with:
- Summary (files analyzed, issues, warnings, score)
- Per-file analysis results
- Check results (compileall, pytest, platformio)
- Recommendations
- Overall status (PASSED/FAILED)

### detect_errors.py

Detects bugs and potential failures with real checks:
- **Pytest**: Runs all tests again for validation
- **py_compile**: Syntax check on all Python files
- **TODO/FIXME grep**: Finds all TODO/FIXME/HACK/XXX comments
- **Static patterns**: Checks for null pointers, buffer issues, resource leaks

**Usage:**
```bash
python scripts/ai_agent/detect_errors.py '{"name":"task_name"}'
```

**Output:** `scripts/ai_agent/output/detect_errors.json` with:
- Summary (files checked, errors, warnings)
- Pytest results with failed test list
- Py_compile syntax errors
- TODO/FIXME locations by file
- Auto-fix suggestions

### generate_patterns.py

Generates real LED patterns with actual code and data:
- **Creates 8 pattern types**: rainbow_cycle, sine_wave, comet_tail, fire_effect, plasma_wave, pulse_beat, noise_shimmer, spiral_trace
- **Outputs both formats**:
  - `.h` files: C++ header files ready for firmware integration
  - `.json` files: Pattern data with parameters, waveforms, color maps
- **Real pattern data**: Not placeholders - actual color arrays, waveforms, heat maps

**Usage:**
```bash
# Generate random patterns
python scripts/ai_agent/generate_patterns.py '{"num_patterns":4}'

# Generate specific patterns
python scripts/ai_agent/generate_patterns.py '{"pattern_ids":["rainbow_cycle","fire_effect"]}'
```

**Output:** `scripts/ai_agent/output/generated_patterns/`
- `pattern_*.h` - C++ header files with FastLED code
- `pattern_*.json` - JSON with pattern data (colors, waveforms, parameters)

### agent_dispatcher.py

Coordinates all agents and processes tasks from `.github/tasks/`:
- **Reads task files**: Parses markdown files in `.github/tasks/`
- **Extracts Type field**: `**Type**: analysis|test|pattern|firmware`
- **Routes to scripts**:
  - `Type: analysis` → `analyze_code.py`
  - `Type: test` → `detect_errors.py`
  - `Type: pattern` → `generate_patterns.py`
  - `Type: firmware` → `generate_patterns.py`
- **Read-only**: Never mutates task files, only reads them

**Usage:**
```bash
python scripts/ai_agent/agent_dispatcher.py
```

**Output:** `scripts/ai_agent/output/`
- `dispatcher_summary.json` - Task results and timing
- `dispatcher.log` - Detailed execution log

## Task System

Tasks are markdown files in `.github/tasks/` with metadata:

```markdown
# Task: Example Task

**Priority**: High  
**Type**: analysis  
**Status**: ready

## Description
Task description here

## Requirements
- [ ] Requirement 1
- [ ] Requirement 2
```

**Available Task Templates:**
- `analysis-full-repository.md` - Complete code analysis
- `firmware-integration-testing.md` - PlatformIO testing
- `generate-new-patterns.md` - Create new LED patterns

**Task Type Mapping:**
- `Type: analysis` → Runs code analysis with compileall, pytest, PlatformIO
- `Type: test` → Runs error detection with pytest, py_compile, TODO finder
- `Type: pattern` → Generates LED pattern files
- `Type: firmware` → Generates LED pattern files (firmware-related)

## GitHub Actions Workflow

File: `.github/workflows/ai-agent-automation.yml`

**Jobs:**

1. **ai_agent_dispatcher** - Runs on schedule, workflow_dispatch, or push to main
   - Installs dependencies from `requirements.txt`
   - Runs `agent_dispatcher.py` to process all tasks
   - Uploads output artifacts

2. **analyze_code** - Runs on push or pull request
   - Runs `analyze_code.py` for comprehensive analysis
   - Uploads analysis results artifact

3. **detect_errors** - Runs on push or pull request
   - Runs `detect_errors.py` for error detection
   - Uploads error detection results artifact

4. **generate_patterns** - Runs on schedule or manual trigger
   - Generates 4 new LED patterns
   - Uploads generated pattern files

5. **on_demand_task** - Runs on issue comments with `/ai-agent`
   - Parses comment for: `/ai-agent analyze|pattern|test`
   - Runs corresponding script
   - Uploads task results

**Environment Variables:**
- `SKIP_PIO=1` - Skip PlatformIO build (used by default in CI)

## Output Files

All results saved to `scripts/ai_agent/output/`:

```
output/
├── analyze_code.json           # Code analysis results
├── detect_errors.json          # Error detection results  
├── dispatcher_summary.json     # Task dispatch summary
├── dispatcher.log              # Dispatcher execution log
└── generated_patterns/         # LED pattern files
    ├── pattern_rainbow_cycle.h     # C++ header
    ├── pattern_rainbow_cycle.json  # Pattern data
    ├── pattern_fire_effect.h
    ├── pattern_fire_effect.json
    └── ...
```

## Pattern Generation Details

Each generated pattern includes:

**C++ Header (.h):**
- FastLED-compatible function signature
- Complete implementation with HSV color space
- Usage example in comments
- Implementation notes

**JSON Data (.json):**
- Pattern metadata (id, name, description)
- Parameters with default values
- Actual pattern data:
  - Color arrays (HSV values for 31 LEDs)
  - Waveforms (sine wave samples)
  - Heat maps (fire effect)
  - Phase patterns (pulse beat)
- Code snippet for reference

**Example: rainbow_cycle.json**
```json
{
  "id": "rainbow_cycle",
  "name": "Rainbow Cycle",
  "pattern_data": {
    "type": "rainbow_cycle",
    "frames": 60,
    "speed": 50,
    "colors": [
      {"hue": 0, "saturation": 255, "brightness": 255},
      {"hue": 8, "saturation": 255, "brightness": 255},
      ...
    ]
  }
}
```

## Adding New Tasks

1. Create `.github/tasks/your-task-name.md`
2. Add task metadata:
   ```markdown
   **Type**: analysis|test|pattern|firmware
   **Priority**: High|Medium|Low
   **Status**: ready
   ```
3. Run dispatcher: `python scripts/ai_agent/agent_dispatcher.py`
4. Check output in `scripts/ai_agent/output/`

**Note:** Dispatcher does not mutate task files - they remain in place for future runs.

## On-Demand Execution

Comment on any GitHub issue with:

```
/ai-agent analyze   # Run code analysis
/ai-agent pattern   # Generate patterns  
/ai-agent test      # Run error detection
```

Results uploaded as workflow artifacts.

## Dependencies

File: `scripts/ai_agent/requirements.txt`

```
Pillow>=9.0.0        # Image processing (for tests)
PyYAML>=6.0          # Task file parsing
pytest>=7.0.0        # Testing framework
pytest-cov>=4.0.0    # Coverage reporting
platformio>=6.0.0    # Firmware builds (optional, skip with SKIP_PIO=1)
```

## Troubleshooting

### Python Not Found
```bash
python --version  # Must be 3.10+
```

### PlatformIO Build Fails
```bash
# Skip PlatformIO in local dev
SKIP_PIO=1 python scripts/ai_agent/analyze_code.py '{"name":"test"}'
```

### No Tasks Found
- Check `.github/tasks/` directory exists
- Ensure task files have `.md` extension
- Verify `**Type**:` field is present

### Pytest Not Finding Tests
- Tests must be in `examples/` directory
- Test files must match `test_*.py` pattern
- Install dependencies: `pip install -r scripts/ai_agent/requirements.txt`

## Success Metrics

| Metric | Target | Current |
|--------|--------|---------|
| Code Quality Score | >80 | Variable (see analyze_code.json) |
| Test Pass Rate | 100% | 22/22 tests passing |
| Critical Errors | 0 | Check detect_errors.json |
| Patterns Generated | 8 | All 8 patterns available |

## Architecture

```
User/GitHub Actions
       ↓
agent_dispatcher.py (reads .github/tasks/*.md)
       ↓
    ┌──┴──┬──────────┬────────────┐
    ↓     ↓          ↓            ↓
analyze detect  generate    common.py
_code   _errors _patterns   (shared utils)
    ↓     ↓          ↓
  JSON   JSON   .h + .json
 output output   patterns
```

## License

MIT License - Free for any use.

## Support

- GitHub Issues for bugs
- Check output JSON files for detailed error info
- Review `scripts/ai_agent/output/dispatcher.log` for execution details
- See individual script source for implementation details
