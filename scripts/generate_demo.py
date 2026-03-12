#!/usr/bin/env python3
"""
generate_demo.py — Build docs/index.html from espibright/html.h.

Extracts the HTML payload from the PROGMEM raw-string literal in html.h and
injects the demo-mode shims (fake fetch API, banner) so the page works as a
self-contained, device-free demo on GitHub Pages.

Demo additions come from two template files kept alongside this script:
  scripts/demo_head_inject.html  — <script> + <style> injected before </head>
  scripts/demo_body_inject.html  — banner <div> injected at the top of <body>

Usage:
    python3 scripts/generate_demo.py          # generate docs/index.html
    python3 scripts/generate_demo.py --check  # verify it is up to date (CI)

Exit codes:
    0  success / file is up to date
    1  --check: file is out of date or missing
"""

import argparse
import sys
from pathlib import Path

SCRIPTS_DIR = Path(__file__).resolve().parent
REPO_ROOT   = SCRIPTS_DIR.parent
SRC         = REPO_ROOT / "espibright" / "html.h"
DEST        = REPO_ROOT / "docs" / "index.html"
HEAD_INJECT = SCRIPTS_DIR / "demo_head_inject.html"
BODY_INJECT = SCRIPTS_DIR / "demo_body_inject.html"

OPEN_DELIM  = 'R"HTMLEOF('
CLOSE_DELIM = ')HTMLEOF"'


def extract_html(source: Path) -> str:
    text = source.read_text(encoding="utf-8")

    open_pos = text.find(OPEN_DELIM)
    if open_pos == -1:
        raise ValueError(f"Opening delimiter '{OPEN_DELIM}' not found in {source}")
    html_start = open_pos + len(OPEN_DELIM)

    close_pos = text.find(CLOSE_DELIM, html_start)
    if close_pos == -1:
        raise ValueError(f"Closing delimiter '{CLOSE_DELIM}' not found in {source}")

    return text[html_start:close_pos]


def inject_demo(html: str, head_snippet: str, body_snippet: str) -> str:
    """Insert demo shims into the extracted HTML."""
    # Inject before </head>
    if "</head>" not in html:
        raise ValueError("</head> not found in extracted HTML")
    html = html.replace("</head>", head_snippet + "</head>", 1)

    # Inject at the top of <body> (after the opening tag)
    if "<body>" not in html:
        raise ValueError("<body> not found in extracted HTML")
    html = html.replace("<body>\n", "<body>\n" + body_snippet, 1)

    return html


def build(src: Path = SRC, dest: Path = DEST,
          head_inject: Path = HEAD_INJECT,
          body_inject: Path = BODY_INJECT) -> str:
    html        = extract_html(src)
    head_snippet = head_inject.read_text(encoding="utf-8")
    body_snippet = body_inject.read_text(encoding="utf-8")
    result      = inject_demo(html, head_snippet, body_snippet)
    return result.rstrip("\n") + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check mode: exit 1 if docs/index.html would change",
    )
    args = parser.parse_args()

    output = build()

    if args.check:
        if not DEST.exists():
            print(f"FAIL: {DEST} does not exist", file=sys.stderr)
            sys.exit(1)
        current = DEST.read_text(encoding="utf-8")
        if current != output:
            print(
                f"FAIL: {DEST} is out of date.\n"
                f"Run:  python3 scripts/generate_demo.py",
                file=sys.stderr,
            )
            sys.exit(1)
        print(f"OK: {DEST} is up to date")
        sys.exit(0)

    DEST.parent.mkdir(parents=True, exist_ok=True)
    DEST.write_text(output, encoding="utf-8")
    print(f"Written {len(output):,} bytes to {DEST.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()
