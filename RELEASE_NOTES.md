Local, private face anonymization for photos. FaceVeil runs entirely on your machine — no uploads, no telemetry, no accounts.

## What's New in 0.1.1

A maintenance release focused on **security**, **stability**, and **quality-of-life** improvements. The feature set is unchanged — everything you did in 0.1.0 works the same way, just faster, safer, and with better feedback.

### Security
- **Path-traversal hardened** — symlinks inside the output directory are resolved and rejected if they point outside. Output directory creation errors no longer pass silently.
- **Decode-bomb protection** — input files over 1 GiB or images larger than 200 megapixels are skipped with a clear log message instead of exhausting memory.
- **macOS entitlements minimized** — removed `disable-library-validation`, `allow-unsigned-executable-memory`, and `allow-jit`. Bundled dylibs are signed with the same Developer ID, so library validation now works as intended.
- **Compiler hardening** — added `-fstack-protector-strong`, `_FORTIFY_SOURCE=2`, PIE, `relro/now/noexecstack` (Linux) and `/guard:cf /GS /sdl` (MSVC).
- **Model output validation** — ONNX output tensors are checked for correct type, non-empty shape, and element-count overflow before being decoded, so a malformed model can't crash the app.

### Stability
- **Atomic file writes** — each processed image is written to a sibling temp file and renamed, so an interrupted save never leaves a half-written file behind.
- **Cooperative thread shutdown** — replaced `QThread::terminate()` with a cancel-flag checked between images, eliminating a class of potential crashes when closing during processing.
- **Safer directory scans** — permission-denied errors on individual subfolders no longer abort the whole scan, and symlink cycles can't cause infinite traversal.
- **Case-insensitive duplicate detection** — dragging the same folder twice, or folders that differ only by case on APFS/NTFS, now collapses to a single entry.
- **Input = output guard** — refuses to run when the output folder is the same as (or inside) any input, to prevent accidental overwrites of originals.

### Performance
- **Detector session reuse** — running a second batch with the same model no longer reloads the ONNX session. Saves several seconds per run on slower disks.
- **Review mode on huge images** — images larger than 1600 px on the long edge are downscaled for the review dialog. The actual mosaic still runs on the full-resolution file.
- **Cheaper BGR→QImage conversion** — eliminated an unnecessary `cvtColor` pass by using `QImage::Format_BGR888` directly.

### Quality of Life
- **Per-image progress stages** — the status bar now updates through *Loading → Detecting → Reviewing → Applying mosaic → Saving* for each image, so the UI no longer appears frozen on a single large photo.
- **Undo / Redo in Review mode** — add, delete, or toggle boxes freely; use **⌘Z / ⌘⇧Z** (or Ctrl+Z / Ctrl+Y on Linux/Windows) or the on-screen **Undo** / **Redo** buttons. Up to 64 steps of history.
- **Settings persistence** — last-used model, output folder, Review toggle, recursive toggle, all four advanced options, and window geometry are restored on next launch.
- **Better drag-and-drop feedback** — drops that aren't local files or folders show the standard "not allowed" cursor instead of silently being ignored.
- **Log size bound** — the Activity log keeps the most recent 5 000 lines so a long batch can't grow the window's memory forever.

### Packaging
- **Windows script more robust** — Qt root is now detected via `bin\qmake.exe` instead of a directory-name guess, ONNX Runtime is located across three common layouts, OpenCV debug DLLs are filtered out, and every step validates its output.
- **macOS script portable** — uses `brew --prefix` to locate Homebrew (works on both Apple Silicon and Intel Macs), validates the signing identity up front, and verifies required tools before building.
- **Notarization script safer** — adds an optional `NOTARY_TIMEOUT` (default 3600 s), surfaces log-fetch failures instead of hiding them, and warns when credentials are passed via plaintext environment variables.

## Features

- Drag & drop images or folders, with optional recursive folder scan
- Bundled SCRFD face detection: **Fast** (SCRFD 2.5G) and **Accurate** (SCRFD 10G), plus custom ONNX model support
- Batch mosaic processing — original files are never modified
- **Review mode** with Undo/Redo — inspect each image before saving, click a detected box to exclude it, drag an empty area to add a face the model missed
- Advanced Options (score threshold, NMS threshold, mosaic block size, face padding) with one-click reset to defaults
- Settings and window layout remembered across launches
- Minimal, light-themed UI

## Downloads

### macOS — Apple Silicon (arm64)

- `FaceVeil-0.1.1-arm64.dmg` — signed with Developer ID and notarized by Apple (no Gatekeeper warnings)
- Requires **macOS 12 Monterey or later** on Apple Silicon

### Windows

Not yet available. A Windows build will follow in a later release.

### Intel Mac

Not included in this release. arm64 only for now.

## Install (macOS)

1. Download `FaceVeil-0.1.1-arm64.dmg`
2. Open the DMG and drag **FaceVeil** into **Applications**
3. Launch from Launchpad or Spotlight

## Upgrading from 0.1.0

Just replace the old app in **Applications** with the new one. Your saved settings carry over automatically (they're stored under `~/Library/Preferences/com.faceveil.FaceVeil.plist`).

## How to Use

1. Drag images or folders into the window — or use **Add Images** / **Add Folder**
2. Pick **Fast** or **Accurate** model (or load your own `.onnx`)
3. Choose an output folder (defaults to `~/Pictures/FaceVeil`) — it must not be inside any input folder
4. (Optional) Enable **Review each image** to inspect each result before saving
5. Click **Start**

Processed copies are written to the output folder, preserving subfolder structure. Originals are never touched.

### Review mode at a glance

| Color | Meaning | Action |
|-------|---------|--------|
| Amber | Detected face (will be mosaicked) | Click to exclude |
| Gray + red X | Excluded detection | Click to re-include |
| Blue | Manually added box | Click to delete |

Drag an empty area to add a new mosaic box. Use **⌘Z** / **⌘⇧Z** to undo or redo.

## Known Limitations

- **Apple Silicon only** in this release.
- Face detection is probabilistic — small, side-profile, heavily occluded, or motion-blurred faces may be missed. Use **Review mode** to catch these manually.
- HEIC input depends on your system's OpenCV build and is not guaranteed.
- Very large images (> 200 MP) or files over 1 GiB are skipped by design.

## Privacy

FaceVeil reads from disk, processes locally, and writes to disk. No network calls are made during face detection or mosaic processing.

## Notes on Bundled Models

The bundled SCRFD models come from [InsightFace](https://github.com/deepinsight/insightface) and are licensed for **non-commercial research use only**. Replace with a commercially-licensed model if you plan to redistribute commercially.

## Credits

- Face detection: [SCRFD](https://insightface.ai/scrfd) (InsightFace)
- Runtime: Qt 6, OpenCV 4, ONNX Runtime
