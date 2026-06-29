#!/bin/bash -eu
# Interactive TUI to pick an exercise and flash it to the board.
# Shows every exercise; dual-core and sourceless ones are marked unsupported
# and cannot be selected. The chosen exercise is built (via the Makefile) and
# flashed with st-flash.

HERE="$(dirname "$0")/.."
SINGLEDIR="$HERE/exercises/singlecore"
DUALDIR="$HERE/exercises/dualcore"

# pick a menu backend
if command -v whiptail >/dev/null 2>&1; then
	MENU=whiptail
elif command -v dialog >/dev/null 2>&1; then
	MENU=dialog
else
	echo "error: need 'whiptail' or 'dialog' installed for the menu" >&2
	echo "       (sudo apt install whiptail)" >&2
	exit 1
fi

# Build the menu by scanning both exercise trees. The directory is the
# classification: singlecore/ exercises are flashable; dualcore/ ones build two
# images and need option bytes to run, so they're shown but not selectable.
items=()        # tag + description pairs for the menu
declare -A buildable=()
for dir in "$SINGLEDIR"/[0-9]*_*/; do
	[ -d "$dir" ] || continue
	name="$(basename "$dir")"
	if [ -f "$dir/main.c" ]; then
		items+=("$name" "single-core")
		buildable["$name"]=1
	else
		items+=("$name" "(no source - unsupported)")
	fi
done
for dir in "$DUALDIR"/[0-9]*_*/; do
	[ -d "$dir" ] || continue
	name="$(basename "$dir")"
	items+=("$name" "(dual-core - not flashable yet)")
done

if [ "${#items[@]}" -eq 0 ]; then
	echo "error: no exercises found under $SINGLEDIR or $DUALDIR" >&2
	exit 1
fi

# Show the menu. The selected tag is printed on stderr by whiptail/dialog, so
# capture it via a swap of stdout/stderr (3>&1 1>&2 2>&3).
choice="$("$MENU" --title "Flash an exercise" \
	--menu "Choose an exercise to build and flash:" \
	20 60 12 \
	"${items[@]}" \
	3>&1 1>&2 2>&3)" || { echo "cancelled."; exit 0; }

[ -n "$choice" ] || { echo "cancelled."; exit 0; }

if [ -z "${buildable[$choice]:-}" ]; then
	echo "error: '$choice' is not flashable by this build system" >&2
	echo "       (dual-core exercises and ones without source are unsupported)" >&2
	exit 1
fi

echo "==> building and flashing $choice"
exec "$HERE/scripts/flash_exercise.sh" "$choice"
