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
import urllib.error
import urllib.request
from pathlib import Path

CUBEIDE_URL = "https://www.st.com/en/development-tools/stm32cubeide.html"
LOGIN_HINT = "https://www.st.com/content/st_com/en/user/login.html"
REACH_URL = "https://www.st.com/favicon.ico"  # light endpoint for preflight


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def check_reachable(url=REACH_URL, timeout=12):
    """Preflight: is st.com actually reachable? st.com's Akamai edge can reset
    connections (e.g. after repeated automated hits / bot rate-limiting), which
    otherwise shows up only as a confusing "site can't be reached" mid-run.
    Returns (ok: bool, detail: str)."""
    req = urllib.request.Request(url, method="GET",
                                 headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return True, f"HTTP {resp.status}"
    except urllib.error.HTTPError as e:
        # A real HTTP response (even 403/404) means the host is reachable.
        return True, f"HTTP {e.code}"
    except Exception as e:
        return False, f"{type(e).__name__}: {e}"


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
            # Prefer the first VISIBLE match: a selector may match several
            # elements where the first in the DOM is hidden (st.com renders one
            # hidden download block per OS variant), so .first alone can wedge.
            loc = page.locator(sel).locator("visible=true").first
            loc.wait_for(state="visible", timeout=per)
            loc.click()
            return True
        except Exception:
            continue
    return False


def select_linux_os(page):
    """In the Get-Software table the OS is chosen via a native <select class=swos>
    (id swos_0); picking Linux there reveals the row's Get-latest button. Select
    Linux in that dropdown and fire change events so the page's JS reacts.
    Returns True if Linux was selected."""
    sel = page.locator("select.swos, #swos_0").first
    try:
        sel.wait_for(state="attached", timeout=8000)
    except Exception:
        print("    [select_linux_os] no swos OS dropdown found", file=sys.stderr)
        return False

    # The swos OS list is populated by JS, often only AFTER a version is chosen.
    # So pick a version in swversion first (and fire change) to trigger it.
    ver = page.locator("select.swversion, #swversion_0").first
    try:
        ver_opts = ver.locator("option").all_inner_texts()
        first_ver = next((o for o in ver_opts if o.strip() and o.strip() != "-"),
                         None)
        if first_ver:
            ver.select_option(label=first_ver.strip())
            ver.evaluate("el => el.dispatchEvent(new Event('change',{bubbles:true}))")
            print(f"    [select_linux_os] picked version {first_ver.strip()}",
                  file=sys.stderr)
    except Exception as e:
        print(f"    [select_linux_os] version pick skipped: {e}", file=sys.stderr)

    # Poll for the swos options to populate (JS may load them asynchronously).
    opts = []
    for _ in range(12):  # up to ~6s
        try:
            opts = sel.locator("option").all_inner_texts()
        except Exception:
            opts = []
        if any("linux" in (o or "").lower() for o in opts):
            break
        page.wait_for_timeout(500)
    # If still empty, the list may load on focus/click of the OS select itself.
    if not any("linux" in (o or "").lower() for o in opts):
        for action in ("focus", "click"):
            try:
                getattr(sel, action)(timeout=2000) if action == "click" \
                    else sel.focus(timeout=2000)
            except Exception:
                pass
            page.wait_for_timeout(1200)
            try:
                opts = sel.locator("option").all_inner_texts()
            except Exception:
                opts = []
            if any("linux" in (o or "").lower() for o in opts):
                break
    print(f"    [select_linux_os] swos options: {opts}", file=sys.stderr)
    linux_label = next((o for o in opts if "linux" in (o or "").lower()), None)

    # Prefer the generic Linux installer over the .deb if both are offered.
    generic = next((o for o in opts
                    if "linux" in o.lower() and "deb" not in o.lower()), None)
    linux_label = generic or linux_label
    if not linux_label:
        # Diagnose how swos is wired (data-* url / onchange / sibling) so we can
        # see what actually populates it.
        try:
            html = sel.evaluate("el => el.outerHTML.slice(0, 400)")
            parent = sel.evaluate("el => el.parentElement ? "
                                  "el.parentElement.outerHTML.slice(0,500) : ''")
            print(f"    [select_linux_os] swos still empty. outerHTML:\n      "
                  f"{html}", file=sys.stderr)
            print(f"    [select_linux_os] swos parent:\n      {parent}",
                  file=sys.stderr)
        except Exception as e:
            print(f"    [select_linux_os] diag failed: {e}", file=sys.stderr)
        return False

    try:
        sel.select_option(label=linux_label.strip())
    except Exception:
        try:
            sel.select_option(value=linux_label.strip())
        except Exception:
            return False

    # Fire change/input so the page's JS reveals the Get-latest button.
    try:
        sel.evaluate(
            "el => { el.dispatchEvent(new Event('change',{bubbles:true}));"
            "        el.dispatchEvent(new Event('input',{bubbles:true})); }")
    except Exception:
        pass
    print(f"    [select_linux_os] selected: {linux_label.strip()}", file=sys.stderr)
    return True


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
            "a, button, input[type=submit], input[type=button], [role=button], "
            "select, [class*=dropdown], [class*=select]",
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
    ap.add_argument("--dest",
                    default=os.environ.get("CUBEIDE_DEST", str(repo_root() / "download")),
                    help="directory to save the installer into (default: <repo>/download)")
    ap.add_argument("--headed", action="store_true",
                    help="show the browser window (default: headless)")
    ap.add_argument("--timeout", type=int, default=120,
                    help="overall navigation timeout in seconds (default: 120)")
    ap.add_argument("--skip-reachability", action="store_true",
                    help="skip the st.com reachability preflight check")
    args = ap.parse_args()

    dest = Path(args.dest).expanduser().resolve()
    dest.mkdir(parents=True, exist_ok=True)

    user = get_username()
    password = get_password()
    print(f"==> STM32CubeIDE download as {user} (password from pass/env, not shown)")
    print(f"    saving installer to: {dest}")

    # Preflight: bail early (and clearly) if st.com isn't reachable, rather than
    # failing deep in the browser flow with a vague "site can't be reached".
    if not args.skip_reachability:
        ok, detail = check_reachable()
        if not ok:
            print(f"error: st.com is not reachable right now ({detail}).",
                  file=sys.stderr)
            print("       This is usually temporary - st.com's CDN can reset "
                  "connections after repeated automated requests (rate-limit / "
                  "bot protection). Wait a while and retry; check recovery with:",
                  file=sys.stderr)
            print(f"         curl -sS -o /dev/null -w '%{{http_code}}\\n' "
                  f"{REACH_URL}", file=sys.stderr)
            print("       (a 200 means it's back). Use --skip-reachability to "
                  "bypass this check.", file=sys.stderr)
            return 3
        print(f"    st.com reachable ({detail})")

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
        # Force English locale so the site doesn't redirect to another country
        # site (st.com.cn / ja) and rename every control.
        ctx = browser.new_context(
            accept_downloads=True,
            locale="en-US",
            extra_http_headers={"Accept-Language": "en-US,en;q=0.9"},
        )
        page = ctx.new_page()
        page.set_default_timeout(nav_ms)

        # 1) Open the CubeIDE product page and dismiss cookie banner. st.com can
        #    be slow to fire domcontentloaded, so wait only for "commit" (the
        #    navigation response) and cap it, instead of blocking up to nav_ms.
        try:
            page.goto(CUBEIDE_URL, wait_until="commit", timeout=45000)
            # give the page a moment to render, but don't hang on full load
            try:
                page.wait_for_load_state("domcontentloaded", timeout=15000)
            except Exception:
                pass
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
            # Guard: only if the site drifted to a DIFFERENT-language host
            # (st.com.cn or /ja/), go back to the English page. Use a capped,
            # lenient wait so it can't hang.
            if (".com.cn" in page.url) or ("/ja/" in page.url):
                try:
                    page.goto(CUBEIDE_URL, wait_until="commit", timeout=45000)
                    page.wait_for_load_state("domcontentloaded", timeout=15000)
                except Exception:
                    pass

            # 2a) Reveal the "Get Software" table. Use the language-independent
            #     class (getsw_scrollbutton_temp) so this works in any locale.
            click_if_present(page, [
                "a.getsw_scrollbutton_temp",
                "a[href='#section-get-software-table']",
                "a:has-text('Get Software')",
            ], timeout=6000)
            try:
                page.evaluate("window.scrollTo(0, document.body.scrollHeight)")
                page.wait_for_timeout(1200)
            except Exception:
                pass

            # 2b) The "Get latest" button is hidden until an OS is chosen in the
            #     row's OS dropdown. Select Linux to make it appear.
            if select_linux_os(page):
                print("    selected OS = Linux")
            else:
                print("    (could not find an OS dropdown; trying anyway)")
            page.wait_for_timeout(1500)

            # 2c) Now click the first VISIBLE "Get latest". Use the
            #     language-independent id/class first (text last) so a non-EN
            #     locale can't break it.
            clicked = click_if_present(page, [
                "#initLightDownload",
                "a.license-accept",
                "a:has-text('Get latest')",
            ], timeout=8000)
            if not clicked:
                print("error: could not click a visible 'Get latest' control "
                      "after selecting Linux. The page is dumped below:",
                      file=sys.stderr)
                dump_page(page, "CubeIDE page (after OS select)")
                return 3

            # 3) Licence agreement lightbox -> Accept (class accept-li,
            #    language-independent).
            page.wait_for_timeout(800)
            click_if_present(page, [
                "a.accept-li",
                "a:has-text('Accept')",
                "button:has-text('Accept')",
            ], timeout=8000)

            # 4) If a login/guest dialog appears (only when NOT already logged
            #    in), authenticate with the My ST credentials. If already logged
            #    in, this dialog is skipped and the download starts directly.
            page.wait_for_timeout(800)
            login_btn = page.locator(
                ".email-login:has-text('Log in to MyST'), .email-login").first
            try:
                if login_btn.is_visible(timeout=3000):
                    login_btn.click()
                    # CAS login page: fill username + password and submit.
                    page.locator(
                        "input[type='email'], input[name='username'], #username"
                    ).first.fill(user, timeout=20000)
                    page.locator(
                        "input[type='password'], #password"
                    ).first.fill(password, timeout=20000)
                    click_if_present(page, [
                        "button:has-text('Login')",
                        "button:has-text('Sign in')",
                        "input[type='submit']",
                        "button[type='submit']",
                    ])
                    page.wait_for_timeout(1500)
            except Exception:
                # No login dialog -> already authenticated; proceed.
                pass

            # 5) Capture the download. After accept/login the .sh download
            #    starts; if a final "Download" button is shown, click it.
            try:
                with page.expect_download(timeout=nav_ms) as dl_info:
                    click_if_present(page, [
                        "#emailSubmit",
                        "button:has-text('Download')",
                        "a:has-text('Download latest')",
                        "a[href$='.sh']",
                    ], timeout=8000)
                download = dl_info.value
            except PWTimeout:
                print("error: no download started after accept/login. The "
                      "current page is dumped below; check your My ST login at "
                      f"{LOGIN_HINT}.", file=sys.stderr)
                dump_page(page, "after accept/login")
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
