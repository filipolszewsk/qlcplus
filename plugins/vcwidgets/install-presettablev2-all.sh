#!/usr/bin/env bash
# Build + install Preset Table v2 and Transition panel (macOS).
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
chmod +x "${DIR}/install-vcwidget-dylib.sh"
"${DIR}/presettablev2/install-local.sh"
"${DIR}/presettablev2transition/install-local.sh"
echo ""
echo "Done. Quit and reopen ~/QLC+.app"
echo "  VC → Add → Preset Table v2"
echo "  VC → Add → EFX Engine"
