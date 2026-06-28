#!/usr/bin/env python3
"""Headless, unattended download of the latest STM32CubeIDE generic Linux
installer from st.com using Playwright (no AI, no manual browser steps).

st.com's "My ST" login is a JavaScript/SSO flow, so we drive a real (headless)
browser via Playwright rather than trying to script the auth with curl.

Credentials:
  username - env ST_USER, else parsed from doc/user_and_password.txt
  password - env ST_PASS, else `pass show $ST_PASS_ENTRY`
             (ST_PASS_ENTRY defaults to sites/www.st.com)

Usage:
  scripts/download_cubeide.py [--dest DIR] [--headed] [--timeout SECONDS]

Exit codes: 0 ok, 2 bad/missing credentials, 3 download/flow failure.
"""
import os
import sys

# If playwright isn't importable here but the user's venv has it, re-exec under
# that interpreter. Lets this single script "just work" without a wrapper.
def _reexec_in_venv():
    try:
        import playwright  # noqa: F401
        return  # already good
    except ImportError:
        pass
    venv_py = os.path.expanduser("~/.venv/bin/python")
    if (os.path.isfile(venv_py)
            and os.path.realpath(venv_py) != os.path.realpath(sys.executable)
            and not os.environ.get("_CUBEIDE_REEXEC")):
        os.environ["_CUBEIDE_REEXEC"] = "1"
        os.execv(venv_py, [venv_py, os.path.abspath(__file__), *sys.argv[1:]])


_reexec_in_venv()

import argparse
import re
import subprocess
from pathlib import Path

CUBEIDE_URL = "https://www.st.com/en/development-tools/stm32cubeide.html"
LOGIN_HINT = "https://www.st.com/content/st_com/en/user/login.html"


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def get_username() -> str:
    if os.environ.get("ST_USER"):
        return os.environ["ST_USER"].strip()
    cred = repo_root() / "doc" / "user_and_password.txt"
    if cred.is_file():
        for line in cred.read_text().splitlines():
            m = re.match(r"\s*user:\s*(.+?)\s*$", line, re.IGNORECASE)
            if m:
                return m.group(1).strip()
    sys.exit("error: no ST_USER set and no 'user:' line in doc/user_and_password.txt")


def get_password() -> str:
    if os.environ.get("ST_PASS"):
        return os.environ["ST_PASS"]
    entry = os.environ.get("ST_PASS_ENTRY", "sites/www.st.com")
    try:
        out = subprocess.run(
            ["pass", "show", entry],
            check=True, capture_output=True, text=True,
        )
    except FileNotFoundError:
        sys.exit("error: 'pass' not installed (sudo apt install pass)")
    except subprocess.CalledProcessError:
        sys.exit(f"error: cannot read password from 'pass show {entry}' "
                 "(entry present? gpg unlocked?)")
    pw = out.stdout.splitlines()[0] if out.stdout else ""
    if not pw:
        sys.exit(f"error: 'pass show {entry}' returned an empty password")
    return pw


def click_if_present(page, selectors, timeout=2500):
    """Best-effort click of the first matching selector; ignore if absent.

    timeout is the TOTAL budget, split across the selectors, so a long list of
    non-matching selectors can't hang for timeout*len(selectors).
    """
    per = max(500, int(timeout / max(1, len(selectors))))
    for sel in selectors:
        try:
            loc = page.locator(sel).first
            loc.wait_for(state="visible", timeout=per)
            loc.click()
            return True
        except Exception:
            continue
    return False


def dump_page(page, label):
    """Print the page's clickable elements (download-relevant ones first) and
    save a screenshot, to help diagnose a missing selector."""
    print(f"--- {label}: {page.url}", file=sys.stderr)
    # Scroll to the bottom first so lazily-loaded / below-the-fold content (the
    # "Get Software" table) is present in the DOM before we enumerate.
    try:
        page.evaluate("window.scrollTo(0, document.body.scrollHeight)")
        page.wait_for_timeout(1500)
    except Exception:
        pass
    try:
        rows = page.eval_on_selector_all(
            "a, button, input[type=submit], input[type=button], [role=button]",
            r"""els => els.map(e => ({
                    tag: e.tagName.toLowerCase(),
                    t: (e.innerText||e.value||e.getAttribute('aria-label')||'').trim().slice(0,70),
                    h: (e.getAttribute('href')||''),
                    id: e.id||'',
                    cls: (e.getAttribute('class')||'').slice(0,40),
                }))""",
        )
    except Exception as e:
        print(f"    (could not enumerate elements: {e})", file=sys.stderr)
        rows = []

    # Surface the likely download/login controls first.
    pat = re.compile(r"get\s*latest|download|get software|get-software|"
                     r"\.sh|cubeide|accept|agree|login|sign in", re.I)
    def interesting(r):
        return bool(pat.search(r["t"]) or pat.search(r["h"]) or pat.search(r["cls"]))
    hits = [r for r in rows if interesting(r)]

    def show(r):
        bits = [f"<{r['tag']}>"]
        if r["t"]:
            bits.append(f"text={r['t']!r}")
        if r["id"]:
            bits.append(f"id={r['id']}")
        if r["h"]:
            bits.append(f"href={r['h']}")
        if r["cls"]:
            bits.append(f"class={r['cls']}")
        print("    " + " ".join(bits), file=sys.stderr)

    if hits:
        print(f"  download/login-relevant elements ({len(hits)}):", file=sys.stderr)
        for r in hits:
            show(r)
    print(f"  all clickable elements ({len(rows)} total, showing up to 120):",
          file=sys.stderr)
    for r in rows[:120]:
        show(r)

    try:
        shot = "/tmp/cubeide_page.png"
        page.screenshot(path=shot, full_page=True)
        print(f"--- saved full-page screenshot to {shot}", file=sys.stderr)
    except Exception:
        pass


def main() -> int:
    ap = argparse.ArgumentParser(description="Download STM32CubeIDE (Linux) from st.com.")
    ap.add_argument("--dest", default=os.environ.get("CUBEIDE_DEST", str(repo_root())),
                    help="directory to save the installer into (default: repo root)")
    ap.add_argument("--headed", action="store_true",
                    help="show the browser window (default: headless)")
    ap.add_argument("--timeout", type=int, default=120,
                    help="overall navigation timeout in seconds (default: 120)")
    args = ap.parse_args()

    dest = Path(args.dest).expanduser().resolve()
    dest.mkdir(parents=True, exist_ok=True)

    user = get_username()
    password = get_password()
    print(f"==> STM32CubeIDE download as {user} (password from pass/env, not shown)")
    print(f"    saving installer to: {dest}")

    try:
        from playwright.sync_api import sync_playwright, TimeoutError as PWTimeout
    except ImportError:
        sys.exit("error: playwright not installed (pip install playwright; "
                 "playwright install chromium)")

    nav_ms = args.timeout * 1000
    with sync_playwright() as p:
        # Prefer the system Google Chrome (channel="chrome"): Playwright's own
        # bundled chromium may be missing or unavailable for newer OSes, but a
        # system chrome works fine. Fall back to bundled chromium if no channel.
        try:
            browser = p.chromium.launch(channel="chrome", headless=not args.headed)
        except Exception:
            try:
                browser = p.chromium.launch(headless=not args.headed)
            except Exception as e:
                sys.exit("error: could not launch a browser. Install Google "
                         "Chrome, or run 'playwright install chromium'.\n"
                         f"       ({type(e).__name__}: {e})")
        ctx = browser.new_context(accept_downloads=True)
        page = ctx.new_page()
        page.set_default_timeout(nav_ms)

        # 1) Open the CubeIDE product page and dismiss cookie banner.
        try:
            page.goto(CUBEIDE_URL, wait_until="domcontentloaded")
        except Exception as e:
            print(f"error: could not load {CUBEIDE_URL}\n"
                  f"       ({type(e).__name__}: {str(e).splitlines()[0]})\n"
                  "       check your network / that st.com is reachable.",
                  file=sys.stderr)
            ctx.close(); browser.close()
            return 3
        click_if_present(page, [
            "button:has-text('Accept')",
            "#onetrust-accept-btn-handler",
            "button:has-text('Agree')",
        ])

        # Steps 2-5 are wrapped so that ANY failure or a Ctrl-C still dumps the
        # current page (links/buttons + screenshot) before exiting - that dump
        # is what lets the selectors be fixed against the live site.
        try:
            # 2) Find and click the "Get latest" / download control for the
            #    generic Linux installer. It is below the fold, so scroll first;
            #    ST labels these variously, so try several selectors.
            try:
                page.evaluate("window.scrollTo(0, document.body.scrollHeight)")
                page.wait_for_timeout(1500)
            except Exception:
                pass
            clicked = click_if_present(page, [
                "a:has-text('Get latest')",
                "button:has-text('Get latest')",
                "button:has-text('Get software')",
                "a:has-text('Get software')",
                "a:has-text('Download latest')",
                "[role=button]:has-text('Get')",
                "tr:has-text('Linux'):has-text('Installer') a:has-text('Get')",
                "a[href*='get-software']",
            ], timeout=8000)
            if not clicked:
                print("error: could not find the CubeIDE Linux 'Get latest' "
                      "control (ST may have changed the layout). The page's "
                      "links/buttons are listed below so the selector can be "
                      "fixed:", file=sys.stderr)
                dump_page(page, "CubeIDE page")
                return 3

            # 3) Licence agreement dialog -> Accept.
            click_if_present(page, [
                "button:has-text('Accept')",
                "input[type='submit'][value*='Accept']",
                "a:has-text('Accept')",
            ], timeout=8000)

            # 4) ST then asks to log in (or shows a "login to download" form).
            #    Fill the My ST credentials.
            try:
                page.locator("input[type='email'], input[name*='mail'], #username") \
                    .first.fill(user, timeout=15000)
                page.locator("input[type='password'], #password") \
                    .first.fill(password, timeout=15000)
                click_if_present(page, [
                    "button:has-text('Login')",
                    "button:has-text('Sign in')",
                    "input[type='submit']",
                    "button[type='submit']",
                ])
            except PWTimeout:
                # Already authenticated (cookies) — continue to the download.
                print("    (no login form shown; assuming existing session)")

            # 5) The accept/login usually triggers the file download. Capture it.
            try:
                with page.expect_download(timeout=nav_ms) as dl_info:
                    # Re-trigger the control in case login navigated away.
                    click_if_present(page, [
                        "a:has-text('Download latest')",
                        "a:has-text('Get latest')",
                        "a[href*='.sh']",
                        "a[href*='stm32cubeide']",
                    ], timeout=8000)
                download = dl_info.value
            except PWTimeout:
                print("error: no download started after login/accept. The login "
                      "may have failed or ST changed the flow. The current page "
                      "is dumped below; check your My ST login at "
                      f"{LOGIN_HINT}.", file=sys.stderr)
                dump_page(page, "after login/accept")
                return 3

            suggested = download.suggested_filename or "stm32cubeide-linux-installer.sh"
            target = dest / suggested
            download.save_as(str(target))
            try:
                os.chmod(target, 0o755)
            except OSError:
                pass
            print(f"==> downloaded: {target}")
            print("    run it yourself to install (needs sudo; installs under "
                  "/opt/st/):")
            print(f"      sudo {target}")
        except KeyboardInterrupt:
            print("\ninterrupted - dumping the current page so the selectors "
                  "can be fixed:", file=sys.stderr)
            dump_page(page, "interrupted on")
            return 130
        except Exception as e:
            print(f"error: unexpected failure ({type(e).__name__}: "
                  f"{str(e).splitlines()[0]}); dumping the current page:",
                  file=sys.stderr)
            dump_page(page, "failed on")
            return 3
        finally:
            ctx.close()
            browser.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
