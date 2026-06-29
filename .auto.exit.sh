# shellcheck shell=bash
# Sourced on leaving this repo: undo what .auto.enter.sh added to PATH.
#
# Sourced, not executed: it edits the current shell's PATH. Do not add `set -e`.

if [ -n "${_ST_AUTO_PATH_DIR:-}" ]; then
	# Strip every occurrence of the dir we added from PATH.
	_new_path=":${PATH}:"
	_new_path="${_new_path//:${_ST_AUTO_PATH_DIR}:/:}"
	_new_path="${_new_path#:}"
	_new_path="${_new_path%:}"
	export PATH="${_new_path}"
	unset _ST_AUTO_PATH_DIR _new_path
fi
