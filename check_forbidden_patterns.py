"""
Pre-build forbidden-pattern checker for the wireless-pov-poi project.

Scans the LittleFS data directory (and optionally src/) for patterns that
historically broke the build or runtime: CDN imports, ES module syntax in
LittleFS-served files, hardcoded IPs, hardcoded WiFi credentials, etc.

Wired into platformio.ini via:
    extra_scripts = pre:check_forbidden_patterns.py

Fails the build (sys.exit(1)) on any violation, with file:line:context output
that matches the format editors use for jump-to-error.

Tune the FORBIDDEN list as new failure modes show up. Each rule has:
  - name:         short id used in error output
  - pattern:      compiled regex
  - applies_to:   list of glob roots, relative to project root
  - extensions:   set of file extensions to check (None = all text)
  - message:      human explanation + the correct alternative
  - allow_marker: if this string appears on the same line, the rule is skipped
                  (escape hatch for cases you've consciously decided are fine)
"""

import os
import re
import sys
from pathlib import Path

# PlatformIO injects `env` via the SCons Import() builtin. When run standalone
# (e.g. `python check_forbidden_patterns.py` for local testing), fall back to
# the script's own directory as the project root so the scan still works.
try:
    Import("env")  # noqa: F821  (provided by PlatformIO)
    PROJECT_ROOT = Path(env["PROJECT_DIR"])  # noqa: F821
    _RUNNING_UNDER_PIO = True
except NameError:
    PROJECT_ROOT = Path(__file__).resolve().parent
    _RUNNING_UNDER_PIO = False

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Default scope: the LittleFS data dir is where most failures land, since that's
# what gets served to browsers and can't pull from CDNs once flashed.
DATA_DIR = "data"
SRC_DIR = "src"
INCLUDE_DIR = "include"

ALLOW_MARKER = "FORBIDDEN_OK"  # put this in a comment to bypass a rule on one line

# Files / directories we never descend into, regardless of rule scope.
SKIP_DIR_NAMES = {
    ".git",
    ".pio",
    ".pioenvs",
    ".piolibdeps",
    ".vscode",
    ".idea",
    "node_modules",
    "build",
    "dist",
    "__pycache__",
    ".cache",
    ".zenflow",
    ".zencoder",
    ".claude",
}

# Binary-ish extensions we never bother to open.
SKIP_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".ico", ".webp",
    ".woff", ".woff2", ".ttf", ".otf", ".eot",
    ".mp3", ".wav", ".ogg", ".mp4", ".webm",
    ".zip", ".gz", ".tar", ".7z", ".bin", ".hex", ".elf", ".o", ".a",
    ".pdf",
}

# IPv4 addresses we tolerate even when hardcoded (documented endpoints / standard).
IPV4_ALLOWLIST = {
    "0.0.0.0",
    "127.0.0.1",
    "255.255.255.255",
    "192.168.4.1",   # documented SoftAP IP for the captive UI
}

# WiFi credential literals we tolerate (documented in README / setup guides).
WIFI_LITERAL_ALLOWLIST = {
    "POV-POI-WiFi",
    "povpoi123",
}

FORBIDDEN = [
    {
        "name": "cdn-import-js",
        "pattern": re.compile(
            r"""import\s+(?:[\w*{}\s,]+\s+from\s+)?["']https?://""",
            re.IGNORECASE,
        ),
        "applies_to": [DATA_DIR],
        "extensions": {".js", ".mjs", ".html", ".htm"},
        "message": (
            "CDN import in LittleFS-served file. The device has no internet "
            "egress when serving the captive UI. Vendor the dependency into "
            "data/ and import it with a relative path."
        ),
    },
    {
        "name": "cdn-script-tag",
        "pattern": re.compile(
            r"""<script\b[^>]*\bsrc\s*=\s*["']https?://""",
            re.IGNORECASE,
        ),
        "applies_to": [DATA_DIR],
        "extensions": {".html", ".htm"},
        "message": (
            "<script src=\"http(s)://...\"> in a served HTML file. The captive "
            "UI is offline-only; vendor the script into data/ and reference it "
            "with a relative src."
        ),
    },
    {
        "name": "cdn-stylesheet-link",
        "pattern": re.compile(
            r"""<link\b[^>]*\bhref\s*=\s*["']https?://""",
            re.IGNORECASE,
        ),
        "applies_to": [DATA_DIR],
        "extensions": {".html", ".htm"},
        "message": (
            "<link href=\"http(s)://...\"> (typically a CDN stylesheet) in a "
            "served HTML file. Vendor the stylesheet into data/ and use a "
            "relative href."
        ),
    },
    {
        "name": "es-module-script-tag",
        "pattern": re.compile(
            r"""<script\b[^>]*\btype\s*=\s*["']module["']""",
            re.IGNORECASE,
        ),
        "applies_to": [DATA_DIR],
        "extensions": {".html", ".htm"},
        "message": (
            "<script type=\"module\"> in a LittleFS-served file. The embedded "
            "HTTP server does not set correct MIME types for .mjs / ES module "
            "resolution; bundle to a single classic script instead."
        ),
    },
    {
        "name": "es-module-bare-import",
        # Top-of-line import/export with a relative or bare specifier. Skips
        # http(s) imports because those are caught by `cdn-import-js` with a
        # more specific message.
        "pattern": re.compile(
            r"""^\s*(?:import|export)\b(?!\s+default\b)[^;]*?["'](?!https?://)[^"']+["']""",
        ),
        "applies_to": [DATA_DIR],
        "extensions": {".js", ".mjs"},
        "message": (
            "ES module import/export in a LittleFS-served JS file. The "
            "embedded HTTP server can't resolve module specifiers reliably; "
            "ship a bundled, classic-script build instead."
        ),
    },
    {
        "name": "hardcoded-ipv4",
        # Generic dotted-quad. Allowlist filtering happens in the scan loop
        # so we can let documented endpoints through without false positives.
        "pattern": re.compile(
            r"""\b(?:(?:25[0-5]|2[0-4]\d|[01]?\d?\d)\.){3}(?:25[0-5]|2[0-4]\d|[01]?\d?\d)\b""",
        ),
        "applies_to": [DATA_DIR, SRC_DIR, INCLUDE_DIR],
        "extensions": {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".ino",
            ".js", ".mjs", ".ts", ".tsx", ".jsx",
            ".html", ".htm", ".css",
            ".py", ".json", ".yml", ".yaml", ".toml", ".ini",
        },
        "message": (
            "Hardcoded IPv4 literal. Read endpoints from config (LittleFS / "
            "NVS) or compile-time defines; if this is a documented address, "
            "add it to IPV4_ALLOWLIST in check_forbidden_patterns.py."
        ),
        # Custom predicate (see _rule_matches) — return False to suppress.
        "filter": lambda match, line: match.group(0) not in IPV4_ALLOWLIST,
    },
    {
        "name": "hardcoded-wifi-credential",
        # Catches assignments like:
        #     ssid     = "..."
        #     password = "..."
        #     WIFI_SSID "..."
        #     WiFi.begin("...", "...")
        # The captured literal is checked against WIFI_LITERAL_ALLOWLIST.
        "pattern": re.compile(
            r"""
            (?:
                \b(?:ssid|password|passwd|psk|wifi_ssid|wifi_pass(?:word)?)\b
                \s*[:=]\s*
                "(?P<lit_a>[^"\n]{1,64})"
            )
            |
            (?:
                \bWiFi\.begin\s*\(\s*
                "(?P<lit_b>[^"\n]{1,64})"
                \s*,\s*
                "(?P<lit_c>[^"\n]{0,64})"
            )
            """,
            re.IGNORECASE | re.VERBOSE,
        ),
        "applies_to": [DATA_DIR, SRC_DIR, INCLUDE_DIR],
        "extensions": {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".ino",
            ".js", ".mjs", ".ts", ".tsx", ".jsx",
            ".py",
        },
        "message": (
            "Hardcoded WiFi credential literal. Move secrets to a "
            "gitignored config header (e.g. secrets.h) or NVS-backed runtime "
            "config; if this is the documented AP credential, add it to "
            "WIFI_LITERAL_ALLOWLIST in check_forbidden_patterns.py."
        ),
        "filter": lambda match, line: any(
            lit and lit not in WIFI_LITERAL_ALLOWLIST
            for lit in (
                match.groupdict().get("lit_a"),
                match.groupdict().get("lit_b"),
                match.groupdict().get("lit_c"),
            )
        ),
    },
]

# ---------------------------------------------------------------------------
# Scanner
# ---------------------------------------------------------------------------

# ANSI colors — only emit when stdout is a TTY, so log files stay clean.
_USE_COLOR = sys.stdout.isatty()
_C_RED = "\033[31m" if _USE_COLOR else ""
_C_YELLOW = "\033[33m" if _USE_COLOR else ""
_C_DIM = "\033[2m" if _USE_COLOR else ""
_C_RESET = "\033[0m" if _USE_COLOR else ""


def _iter_files(root: Path, extensions):
    """Yield files under `root` whose extension is in `extensions`.

    Skips directories listed in SKIP_DIR_NAMES and binary-ish files.
    `extensions` may be None to mean "any text file".
    """
    if not root.exists():
        return
    for dirpath, dirnames, filenames in os.walk(root):
        # Mutate dirnames in-place so os.walk doesn't descend into skip dirs.
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_NAMES]
        for name in filenames:
            ext = os.path.splitext(name)[1].lower()
            if ext in SKIP_EXTENSIONS:
                continue
            if extensions is not None and ext not in extensions:
                continue
            yield Path(dirpath) / name


def _read_text(path: Path):
    """Read a file as text, returning None for files that aren't decodable."""
    try:
        # Most repo files are UTF-8. Fall back to latin-1 so we never crash on
        # an oddly-encoded asset — worst case we get gibberish lines that
        # won't match any of our regexes.
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        try:
            return path.read_text(encoding="latin-1")
        except OSError:
            return None
    except OSError:
        return None


def _rule_files(rule):
    """Resolve a rule's `applies_to` roots to absolute file paths."""
    extensions = rule.get("extensions")
    for rel_root in rule["applies_to"]:
        root = (PROJECT_ROOT / rel_root).resolve()
        yield from _iter_files(root, extensions)


def _rule_matches(rule, line):
    """Yield re.Match objects for a rule against a single line, honoring
    `allow_marker` and any optional `filter` predicate.
    """
    marker = rule.get("allow_marker", ALLOW_MARKER)
    if marker and marker in line:
        return
    predicate = rule.get("filter")
    for match in rule["pattern"].finditer(line):
        if predicate is not None and not predicate(match, line):
            continue
        yield match


def scan(project_root: Path = None):
    """Run all rules and return a list of (rule_name, path, lineno, col,
    snippet, message) violation tuples.

    Exposed as a function so it can be invoked from unit tests / a standalone
    run without going through PlatformIO.
    """
    global PROJECT_ROOT
    if project_root is not None:
        PROJECT_ROOT = Path(project_root).resolve()

    violations = []
    for rule in FORBIDDEN:
        for path in _rule_files(rule):
            text = _read_text(path)
            if text is None:
                continue
            for lineno, line in enumerate(text.splitlines(), start=1):
                # Strip the trailing newline noise but keep inline whitespace
                # so column numbers line up with what the user sees.
                for match in _rule_matches(rule, line):
                    col = match.start() + 1
                    snippet = line.strip()
                    violations.append(
                        (rule["name"], path, lineno, col, snippet, rule["message"])
                    )
    return violations


def _format_violation(name, path, lineno, col, snippet, message):
    """Format a violation as `file:line:col: [rule] message\\n    > snippet`.

    Editors (vim quickfix, VSCode terminal links, GitHub Actions log matchers)
    recognize the leading `file:line:col:` form.
    """
    try:
        rel = path.relative_to(PROJECT_ROOT)
    except ValueError:
        rel = path
    header = (
        f"{_C_RED}{rel}:{lineno}:{col}:{_C_RESET} "
        f"{_C_YELLOW}[{name}]{_C_RESET} {message}"
    )
    body = f"    {_C_DIM}>{_C_RESET} {snippet}"
    return f"{header}\n{body}"


def main():
    violations = scan()
    if not violations:
        print(f"check_forbidden_patterns: OK — scanned {len(FORBIDDEN)} rules under "
              f"{PROJECT_ROOT}")
        return 0

    # Stable, predictable ordering: by file, then line, then column, then rule.
    violations.sort(key=lambda v: (str(v[1]), v[2], v[3], v[0]))
    print(
        f"{_C_RED}check_forbidden_patterns: "
        f"{len(violations)} violation(s) found{_C_RESET}",
        file=sys.stderr,
    )
    for v in violations:
        print(_format_violation(*v), file=sys.stderr)
    print(
        f"\n{_C_DIM}Tip: append a `// {ALLOW_MARKER}` (or language-appropriate "
        f"comment) on the offending line to bypass a rule you've consciously "
        f"accepted.{_C_RESET}",
        file=sys.stderr,
    )
    return 1


# ---------------------------------------------------------------------------
# Entry point — runs immediately at PlatformIO pre-build time, or when this
# file is executed directly for local development.
# ---------------------------------------------------------------------------

if __name__ == "__main__" or _RUNNING_UNDER_PIO:
    _rc = main()
    if _rc != 0:
        sys.exit(_rc)
