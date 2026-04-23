Local, private face anonymization for photos. FaceVeil runs entirely on your machine — no uploads, no telemetry, no accounts.

## Features

- Drag & drop images or folders, with optional recursive folder scan
- Bundled SCRFD face detection: **Fast** (SCRFD 2.5G) and **Accurate** (SCRFD 10G), plus custom ONNX model support
- Batch mosaic processing — original files are never modified
- **Review mode** — inspect each image before saving, click a detected box to exclude it, drag an empty area to add a face the model missed
- Advanced Options (score threshold, NMS threshold, mosaic block size, face padding) with one-click reset to defaults
- Minimal, light-themed UI

## Downloads

### macOS — Apple Silicon (arm64)

- `FaceVeil-0.1.0-arm64.dmg` — signed with Developer ID and notarized by Apple (no Gatekeeper warnings)
- Requires **macOS 12 Monterey or later** on M1 / M2 / M3 / M4 / M5 

### Windows

Not yet available. A Windows build will follow in a later release.

### Intel Mac

Not included in this release. arm64 only for now.

## Install (macOS)

1. Download `FaceVeil-0.1.0-arm64.dmg`
2. Open the DMG and drag **FaceVeil** into **Applications**
3. Launch from Launchpad or Spotlight

## How to Use

1. Drag images or folders into the window — or use **Add Images** / **Add Folder**
2. Pick **Fast** or **Accurate** model (or load your own `.onnx`)
3. Choose an output folder (defaults to `~/Pictures/FaceVeil`)
4. (Optional) Enable **Review each image** to inspect each result before saving
5. Click **Start**

Processed copies are written to the output folder, preserving subfolder structure. Originals are never touched.

### Review mode at a glance

| Color | Meaning | Action |
|-------|---------|--------|
| Amber | Detected face (will be mosaicked) | Click to exclude |
| Gray + red X | Excluded detection | Click to re-include |
| Blue | Manually added box | Click to delete |

Drag an empty area to add a new mosaic box.

## Known Limitations

- **Apple Silicon only** in this release.
- Face detection is probabilistic — small, side-profile, heavily occluded, or motion-blurred faces may be missed. Use **Review mode** to catch these manually.
- HEIC input depends on your system's OpenCV build and is not guaranteed.

## Privacy

FaceVeil reads from disk, processes locally, and writes to disk. No network calls are made during face detection or mosaic processing.

## Notes on Bundled Models

The bundled SCRFD models come from [InsightFace](https://github.com/deepinsight/insightface) and are licensed for **non-commercial research use only**. Replace with a commercially-licensed model if you plan to redistribute commercially.

## Credits

- Face detection: [SCRFD](https://insightface.ai/scrfd) (InsightFace)
- Runtime: Qt 6, OpenCV 4, ONNX Runtime
