#!/usr/bin/env bash
set -euo pipefail

# Submits a signed DMG to Apple's notary service, waits for the result,
# and staples the ticket on success.
#
# Credentials — choose ONE of:
#   1. Keychain profile (recommended, credentials stored once):
#        NOTARY_PROFILE=faceveil-notary
#      Create it first with:
#        xcrun notarytool store-credentials faceveil-notary \
#          --apple-id "you@example.com" \
#          --team-id "ABCDE12345" \
#          --password "app-specific-password"
#
#   2. Environment variables:
#        APPLE_ID=you@example.com
#        TEAM_ID=ABCDE12345
#        APP_PASSWORD=app-specific-password
#
# Usage:
#   scripts/notarize_macos.sh path/to/FaceVeil-x.y.z-arm64.dmg

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <path-to-dmg>"
    exit 1
fi

DMG_PATH="$1"
if [[ ! -f "$DMG_PATH" ]]; then
    echo "❌ DMG not found: $DMG_PATH"
    exit 1
fi

NOTARY_PROFILE="${NOTARY_PROFILE:-}"

SUBMIT_ARGS=(--wait)
if [[ -n "$NOTARY_PROFILE" ]]; then
    SUBMIT_ARGS+=(--keychain-profile "$NOTARY_PROFILE")
    echo "🔐 Using keychain profile: $NOTARY_PROFILE"
else
    : "${APPLE_ID:?APPLE_ID env var is required (or set NOTARY_PROFILE)}"
    : "${TEAM_ID:?TEAM_ID env var is required}"
    : "${APP_PASSWORD:?APP_PASSWORD env var is required}"
    SUBMIT_ARGS+=(--apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_PASSWORD")
    echo "🔐 Using APPLE_ID=$APPLE_ID, TEAM_ID=$TEAM_ID"
fi

echo "📤 Submitting to Apple notary service…"
SUBMIT_OUTPUT="$(xcrun notarytool submit "$DMG_PATH" "${SUBMIT_ARGS[@]}" --output-format plist)"
echo "$SUBMIT_OUTPUT"

STATUS="$(echo "$SUBMIT_OUTPUT" | plutil -extract status raw - 2>/dev/null || echo "unknown")"
SUBMISSION_ID="$(echo "$SUBMIT_OUTPUT" | plutil -extract id raw - 2>/dev/null || echo "")"

if [[ "$STATUS" != "Accepted" ]]; then
    echo "❌ Notarization did not succeed (status: $STATUS)."
    if [[ -n "$SUBMISSION_ID" ]]; then
        echo "📋 Fetching log…"
        LOG_ARGS=()
        if [[ -n "$NOTARY_PROFILE" ]]; then
            LOG_ARGS+=(--keychain-profile "$NOTARY_PROFILE")
        else
            LOG_ARGS+=(--apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_PASSWORD")
        fi
        xcrun notarytool log "$SUBMISSION_ID" "${LOG_ARGS[@]}" || true
    fi
    exit 1
fi

echo "✅ Notarization accepted. Stapling ticket…"
xcrun stapler staple "$DMG_PATH"
xcrun stapler validate "$DMG_PATH"

echo ""
echo "🎉 Done. Distributable DMG: $DMG_PATH"
echo "   Gatekeeper check:"
spctl -a -vv -t open --context context:primary-signature "$DMG_PATH" || true
