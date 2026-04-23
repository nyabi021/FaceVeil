#!/usr/bin/env bash
set -euo pipefail

# Submits a signed DMG to Apple's notary service, waits for the result,
# and staples the ticket on success.
#
# Credentials — choose ONE of:
#   1. Keychain profile (RECOMMENDED — stored once, never appears on the CLI):
#        NOTARY_PROFILE=faceveil-notary
#      Create it first with:
#        xcrun notarytool store-credentials faceveil-notary \
#          --apple-id "you@example.com" \
#          --team-id "ABCDE12345" \
#          --password "app-specific-password"
#
#   2. Environment variables (NOT recommended for interactive shells —
#      values passed on the command line may end up in shell history /
#      process listings. Prefer export in a file you source, or use
#      the keychain profile above):
#        APPLE_ID=you@example.com
#        TEAM_ID=ABCDE12345
#        APP_PASSWORD=app-specific-password
#
# Optional:
#   NOTARY_TIMEOUT   — seconds to wait for Apple's service (default: 3600)
#
# Usage:
#   scripts/notarize_macos.sh path/to/FaceVeil-x.y.z-arm64.dmg

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <path-to-dmg>"
    exit 1
fi

for tool in xcrun plutil; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "❌ Required tool not found: $tool"
        exit 1
    fi
done

DMG_PATH="$1"
if [[ ! -f "$DMG_PATH" ]]; then
    echo "❌ DMG not found: $DMG_PATH"
    exit 1
fi

NOTARY_PROFILE="${NOTARY_PROFILE:-}"
NOTARY_TIMEOUT="${NOTARY_TIMEOUT:-3600}"

# Warn (don't block) when credentials are passed via env vars — the keychain
# profile is safer and gets the same result.
if [[ -z "$NOTARY_PROFILE" ]]; then
    echo "⚠️  Using plaintext env-var credentials."
    echo "   Consider 'xcrun notarytool store-credentials' + NOTARY_PROFILE=<name>."
fi

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

echo "📤 Submitting to Apple notary service (timeout: ${NOTARY_TIMEOUT}s)…"

# Wrap the submit in a timeout so a stalled Apple service can't hang the
# build forever. macOS ships `gtimeout` via coreutils or you can fall back
# to a bash trick with `( sleep ... ) &` — we only reach for gtimeout when
# available to keep this dependency-light.
RUN_WITH_TIMEOUT=()
if command -v gtimeout >/dev/null 2>&1; then
    RUN_WITH_TIMEOUT=(gtimeout "$NOTARY_TIMEOUT")
elif command -v timeout >/dev/null 2>&1; then
    RUN_WITH_TIMEOUT=(timeout "$NOTARY_TIMEOUT")
else
    echo "ℹ️  'timeout'/'gtimeout' not installed — submission will wait indefinitely."
fi

SUBMIT_OUTPUT=""
if ! SUBMIT_OUTPUT="$("${RUN_WITH_TIMEOUT[@]}" xcrun notarytool submit "$DMG_PATH" \
        "${SUBMIT_ARGS[@]}" --output-format plist 2>&1)"; then
    rc=$?
    echo "$SUBMIT_OUTPUT"
    if [[ $rc -eq 124 ]]; then
        echo "❌ Notarization timed out after ${NOTARY_TIMEOUT}s."
    else
        echo "❌ notarytool submit failed (exit $rc)."
    fi
    exit $rc
fi
echo "$SUBMIT_OUTPUT"

STATUS="$(echo "$SUBMIT_OUTPUT" | plutil -extract status raw - 2>/dev/null || echo "unknown")"
SUBMISSION_ID="$(echo "$SUBMIT_OUTPUT" | plutil -extract id raw - 2>/dev/null || echo "")"

if [[ "$STATUS" != "Accepted" ]]; then
    echo "❌ Notarization did not succeed (status: $STATUS)."
    if [[ -n "$SUBMISSION_ID" ]]; then
        echo "📋 Fetching log for submission $SUBMISSION_ID…"
        LOG_ARGS=()
        if [[ -n "$NOTARY_PROFILE" ]]; then
            LOG_ARGS+=(--keychain-profile "$NOTARY_PROFILE")
        else
            LOG_ARGS+=(--apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_PASSWORD")
        fi
        # Surface the log fetch failure instead of swallowing it with `|| true`,
        # otherwise credential errors and service outages look like a clean log.
        if ! xcrun notarytool log "$SUBMISSION_ID" "${LOG_ARGS[@]}"; then
            echo "⚠️  Could not fetch the notarization log (check credentials or submission ID)."
        fi
    else
        echo "⚠️  No submission ID was returned; cannot fetch log automatically."
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
