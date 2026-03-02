# AI Agent — scripts/ai_agent

Deterministic code-analysis and build-check scripts for the **wireless-pov-poi**
repository. They run in CI (GitHub Actions) on every push / pull request and
can also be executed locally without any hardware.

---

## Scripts

| Script | Purpose |
|--------|---------|
| `agent_dispatcher.py` | Orchestrator — runs all sub-agents and produces a merged report |
| `analyze_code.py` | Static analysis: C++ firmware heuristics + Python `compileall` + optional ruff |
| `detect_errors.py` | Build checks: PlatformIO firmware builds + TypeScript `tsc --noEmit` + Python syntax |
| `generate_patterns.py` | Generates LED pattern code snippets/suggestions for the Teensy firmware |

---

## Output

All scripts write their results to `scripts/ai_agent/output/` (git-ignored):

| File | Description |
|------|-------------|
| `output/findings.json` | Machine-readable list of findings with severity, file, line, and suggested fix |
| `output/report.md` | Human-readable Markdown summary of all findings |
| `output/generated_patterns/` | Pattern `.cpp` snippets (only from `generate_patterns.py`) |

---

## Running Locally

### Prerequisites

```bash
# Python 3.10+
python3 --version

# PlatformIO (for firmware builds)
pip install platformio

# Node.js / npm (for TypeScript check)
node --version
npm --version
```

### Install agent dependencies

```bash
pip install -r scripts/ai_agent/requirements.txt
```

### Run individual scripts

```bash
# From the repo root:

# Full orchestrated run (analyze + detect):
python3 scripts/ai_agent/agent_dispatcher.py

# Static analysis only (no builds):
python3 scripts/ai_agent/analyze_code.py

# Build-based error detection (includes PlatformIO):
python3 scripts/ai_agent/detect_errors.py

# Generate LED pattern suggestions:
python3 scripts/ai_agent/generate_patterns.py '{"num_patterns":4}'

# Skip PlatformIO builds (faster iteration):
SKIP_PIO=1 python3 scripts/ai_agent/detect_errors.py
SKIP_PIO=1 python3 scripts/ai_agent/agent_dispatcher.py
```

### Read the report

```bash
cat scripts/ai_agent/output/report.md

# Or pretty-print the JSON findings:
python3 -m json.tool scripts/ai_agent/output/findings.json
```

---

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SKIP_PIO` | `0` | Set to `1` to skip PlatformIO firmware builds |

---

## CI Integration

The scripts are wired to `.github/workflows/ai-agent-automation.yml`:

| Job | Trigger | Script |
|-----|---------|--------|
| `platformio_build` | push / PR | `pio run -e teensy41` + `pio run -e esp32s3` (dedicated build gate) |
| `AI Agent Dispatcher` | push to main / schedule | `agent_dispatcher.py` |
| `AI Code Analysis` | push / PR | `analyze_code.py` |
| `Error Detection` | push / PR | `detect_errors.py` |
| `Pattern Generation` | schedule / manual | `generate_patterns.py` |

The `platformio_build` job **fails the workflow** if any firmware build fails.
The agent script jobs upload their `output/` directory as GitHub Actions artifacts
and use `continue-on-error: true` so findings are always captured.

---

## PlatformIO Build Details

### Teensy 4.1 (from repo root)

```bash
pio run -e teensy41
```

Uses `platformio.ini` at the repo root with `src_dir = teensy_firmware`.

### ESP32-S3 (from `esp32_firmware/`)

```bash
cd esp32_firmware
pio run -e esp32s3
```

Uses `esp32_firmware/platformio.ini`.

> **Note:** The repo-root `platformio.ini` also has an `[env:esp32s3]` entry but
> its `src_dir = teensy_firmware` means it builds Teensy sources. Always run the
> ESP32 build from the `esp32_firmware/` subdirectory.

---

## Adding New Checks

To add a new heuristic check:

1. Open `analyze_code.py` (for static checks) or `detect_errors.py` (for build checks).
2. Add a new function following the existing pattern:
   ```python
   def _check_my_thing(path: Path) -> List[Dict]:
       findings = []
       # ... your logic ...
       findings.append({
           "source": "analyze_code:my_check",
           "severity": "warn",   # "error" | "warn" | "info"
           "file": _rel(path),
           "line_start": lineno,
           "description": "Human-readable description of the issue.",
           "suggested_fix": "How to fix it.",
       })
       return findings
   ```
3. Call your function from `_run_cpp_checks()` (or equivalent) and extend the list.
