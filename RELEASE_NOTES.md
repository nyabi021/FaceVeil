A maintenance release: security, stability, and UX improvements. No feature changes.

## Security
- Path-traversal defence resolves symlinks inside the output tree
- Files > 1 GiB or images > 200 MP are skipped to prevent decode bombs
- Minimised macOS hardened-runtime entitlements
- Compiler hardening flags (`-fstack-protector-strong`, `_FORTIFY_SOURCE=2`, PIE, MSVC `/guard:cf`)
- ONNX model output tensors are validated before decode

## Stability
- Atomic image writes (temp-file + rename)
- Cooperative thread shutdown; `QThread::terminate()` removed
- Directory scan skips permission errors and breaks symlink cycles
- Case-insensitive input de-duplication
- Refuses to run when the output folder is inside any input

## Performance
- SCRFD session is reused across runs with the same model
- Review dialog uses a downscaled preview (≤ 1600 px long edge) for huge images
- Dropped a redundant `cvtColor` in BGR → QImage conversion

## UX
- Per-image progress stages (Loading / Detecting / Reviewing / Applying mosaic / Saving)
- Undo / Redo in Review mode (⌘Z / ⌘⇧Z, 64-step history)
- Settings and window geometry persist across launches
- Drag-and-drop shows the correct cursor for invalid drops
- Activity log capped at 5 000 lines

## Packaging
- Windows script: qmake-based Qt detection, stricter dependency checks
- macOS script: dynamic Homebrew prefix, upfront signing-identity validation
- Notarization script: optional timeout, clearer failure messages

## Download

- **macOS (Apple Silicon)** — `FaceVeil-0.1.1-arm64.dmg`, signed and notarized. Requires macOS 12+.
- **Windows (x64)** — `FaceVeil-0.1.1-windows-x64.zip`. Unzip and run `FaceVeil.exe`. Requires Windows 10 or later.