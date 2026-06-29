# shellcheck shell=bash
# Sourced on entering this repo: put the STM32CubeIDE command-line tools
# (STM32_Programmer_CLI et al.) on PATH so scripts/run.sh, reset.sh, mass_erase.sh
# work without a full path. Undone by .auto.exit.sh.
#
# Sourced, not executed: it edits the current shell's PATH. Do not add `set -e`.

# Reuse the single source of truth for the install location.
source "$(dirname "${BASH_SOURCE[0]}")/scripts/cubeide.sh"

# The CLI lives in the cubeprogrammer plugin's tools/bin; CLI is its full path.
if [ -n "${CLI:-}" ] && [ -x "${CLI}" ]; then
	ST_BIN_DIR="$(dirname "${CLI}")"
	# Prepend only if not already present.
	case ":${PATH}:" in
		*":${ST_BIN_DIR}:"*) ;;
		*) export PATH="${ST_BIN_DIR}:${PATH}" ;;
	esac
	# Remember exactly what we added so .auto.exit.sh can remove just that.
	export _ST_AUTO_PATH_DIR="${ST_BIN_DIR}"
else
	echo ".auto.enter.sh: STM32_Programmer_CLI not found under /opt/st/ (CubeIDE installed?)" >&2
fi
