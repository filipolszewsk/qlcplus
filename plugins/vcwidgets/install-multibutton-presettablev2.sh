#!/usr/bin/env bash
# Build + install MultiButton and Preset Table v2 together (shared iface ABI).
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
chmod +x "${DIR}/install-vcwidget-dylib.sh"
"${DIR}/multibutton/install-local.sh"
"${DIR}/presettablev2/install-local.sh"
echo ""
echo "Done. Quit and reopen ~/QLC+.app"
echo "  Linked widgets: Multi Button + Preset Table v2 must match (iface 1.1)"
