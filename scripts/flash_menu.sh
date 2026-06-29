#!/bin/bash -eu
# Interactive TUI to pick a single-core exercise (and bare-metal vs HAL variant)
# and flash it to the board. Dual-core exercises are shown but not selectable
# (they need option bytes to run). Flashes a PRE-BUILT image via flash_exercise.sh
# (run `make` first); this menu does not build.

HERE="$(dirname "$0")/.."
SINGLEDIR="$HERE/exercises/single_core"
DUALDIR="$HERE/exercises/dual_core"

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

# Every single-core exercise has both a bare and a HAL solution, so they are all
# flashable. (Dual-core exercises need option bytes to run and are not offered.)
items=()        # tag + description pairs for the menu
for dir in "$SINGLEDIR"/[0-9]*_*/; do
	[ -d "$dir" ] || continue
	items+=("$(basename "$dir")" "single-core (bare + HAL)")
done

if [ "${#items[@]}" -eq 0 ]; then
	echo "error: no exercises found under $SINGLEDIR" >&2
	exit 1
fi

# Show the menu. The selected tag is printed on stderr by whiptail/dialog, so
# capture it via a swap of stdout/stderr (3>&1 1>&2 2>&3).
choice="$("$MENU" --title "Flash an exercise" \
	--menu "Choose an exercise to flash:" \
	20 60 12 \
	"${items[@]}" \
	3>&1 1>&2 2>&3)" || { echo "cancelled."; exit 0; }

[ -n "$choice" ] || { echo "cancelled."; exit 0; }

# Second menu: bare-metal vs HAL solution for the chosen exercise.
variant="$("$MENU" --title "Which solution?" \
	--menu "Build '$choice' as:" \
	12 60 2 \
	bare "register-level (bare metal)" \
	hal  "using the STM32 HAL" \
	3>&1 1>&2 2>&3)" || { echo "cancelled."; exit 0; }

[ -n "$variant" ] || { echo "cancelled."; exit 0; }

echo "==> flashing $choice ($variant)"
exec "$HERE/scripts/flash_exercise.sh" "$choice" "$variant"
