# FaceVeil

![Release](https://img.shields.io/github/v/release/n4yabi12/FaceVeil?style=flat&color=6366f1)
![Downloads](https://img.shields.io/github/downloads/n4yabi12/FaceVeil/total?style=flat&color=10b981)
![Last Commit](https://img.shields.io/github/last-commit/n4yabi12/FaceVeil?style=flat&color=f59e0b)
![License](https://img.shields.io/badge/app%20code-MIT-8b5cf6?style=flat)

FaceVeil is a local desktop app for automatically anonymizing faces in image files. Drop in images or folders, choose a face detection model, and FaceVeil writes mosaic-protected copies to an output folder without overwriting the originals.

FaceVeil currently supports macOS and is being prepared for Windows distribution.

## Features

- Batch scan images and folders
- Optional recursive folder scanning
- Local SCRFD ONNX face detection
- Fast and accurate model presets
- Adjustable score threshold, NMS threshold, face padding, and mosaic strength
- Original files are preserved
- Processed copies are written to a user-selected output folder
- No cloud upload or network processing

## Model Presets

FaceVeil looks for these bundled SCRFD ONNX models:

- `2.5g_bnkps.onnx` as `Fast - SCRFD 2.5G`
- `10g_bnkps.onnx` as `Accurate - SCRFD 10G`

You can also select a custom ONNX model from the app with `Model...`.

The app expects an SCRFD-style output layout:

- score tensors for strides 8, 16, and 32
- bbox distance tensors for strides 8, 16, and 32
- optional 5-point landmark tensors

## Supported Images

FaceVeil currently scans:

- `.jpg`
- `.jpeg`
- `.png`
- `.bmp`
- `.tif`
- `.tiff`
- `.webp`

HEIC support depends on the local OpenCV build and is not enabled by default.

## Requirements

### macOS

- macOS on Apple Silicon for the current packaged build
- CMake 3.24+
- Qt 6
- OpenCV 4
- ONNX Runtime
- Homebrew `pkg-config` support for `libonnxruntime`

### Windows

- Windows 10 or later recommended
- CMake 3.24+
- Visual Studio 2022 Build Tools or a compatible MSVC environment
- Qt 6 MSVC kit
- OpenCV 4 Windows build
- ONNX Runtime Windows package

## Build

### macOS

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
open build/FaceVeil.app
```

### Windows

PowerShell example:

```powershell
cmake -S . -B build-windows -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64;C:\opencv\build" `
  -DONNXRUNTIME_ROOT="C:\onnxruntime-win-x64-1.24.4"

cmake --build build-windows --config Release
```

## Package

### macOS

**Local build (ad-hoc signed, for testing on your own machine):**

```bash
./scripts/package_macos.sh
```

Output: `dist/macos/FaceVeil.app`

**Release build (Developer ID signed + notarized DMG):**

1. Install your **Developer ID Application** certificate in Keychain (Xcode → Settings → Accounts → Manage Certificates).
2. One-time: store notary credentials in the keychain.
   ```bash
   xcrun notarytool store-credentials faceveil-notary \
     --apple-id "you@example.com" \
     --team-id "ABCDE12345" \
     --password "app-specific-password"   # generate at appleid.apple.com
   ```
3. Sign, bundle, and create the DMG:
   ```bash
   DEVELOPER_ID="Developer ID Application: Your Name (ABCDE12345)" \
     ./scripts/package_macos.sh
   ```
4. Notarize and staple:
   ```bash
   NOTARY_PROFILE=faceveil-notary \
     ./scripts/notarize_macos.sh dist/macos/FaceVeil-*.dmg
   ```

The resulting `dist/macos/FaceVeil-<version>-arm64.dmg` is ready to upload to GitHub Releases. It will open without Gatekeeper warnings on any Apple Silicon Mac.

The current pipeline targets Apple Silicon (arm64). For Universal binaries, rebuild OpenCV and ONNX Runtime as fat libraries and adjust `CMAKE_OSX_ARCHITECTURES`.

### Windows

Run from PowerShell on Windows:

```powershell
.\scripts\package_windows.ps1 `
  -QtRoot "C:\Qt\6.11.0\msvc2022_64" `
  -OpenCvRoot "C:\opencv\build" `
  -OnnxRuntimeRoot "C:\onnxruntime-win-x64-1.24.4"
```

Output:

```text
dist/windows/FaceVeil
```

The Windows package copies the executable, Qt runtime files, OpenCV DLLs, ONNX Runtime DLL, and local ONNX files from `models/`.

## Repository Layout

```text
FaceVeil/
  include/faceveil/      Public C++ headers
  src/                   App and processing implementation
  scripts/               Packaging scripts
  tools/                 Developer utilities
  models/                Local ONNX model files, ignored by Git
```

## Privacy

FaceVeil is designed for local processing. Images are read from disk, processed locally, and written to the selected output directory. The app does not upload images or model inputs to a server.

## Important Limitations

Face detection is probabilistic. Small faces, side profiles, heavy occlusion, motion blur, extreme lighting, or unusual crops may cause missed detections. Review output images before publishing or sharing them.

## License

FaceVeil application source code is licensed under the MIT License. See [LICENSE](LICENSE).

SPDX identifier: `MIT`

Copyright (c) 2026 Nyabi

Third-party dependencies keep their own licenses, including Qt, OpenCV, and ONNX Runtime.

The bundled SCRFD/InsightFace model files are not covered by FaceVeil's application license. The InsightFace project states that its provided pretrained models are available for non-commercial research purposes only. If you plan to distribute FaceVeil commercially or bundle these models in a public release, verify the model license and obtain any required commercial rights from the model owner.

References:

- [InsightFace GitHub license notice](https://github.com/deepinsight/insightface)
- [InsightFace Model Zoo](https://github.com/deepinsight/insightface/blob/master/model_zoo/README.md)
- [SCRFD project page](https://insightface.ai/scrfd)

## Citation

FaceVeil uses SCRFD-style face detection models. If you use SCRFD in research, cite the original paper:

```bibtex
@misc{guo2021sample,
  title={Sample and Computation Redistribution for Efficient Face Detection},
  author={Jia Guo and Jiankang Deng and Alexandros Lattas and Stefanos Zafeiriou},
  year={2021},
  eprint={2105.04714},
  archivePrefix={arXiv},
  primaryClass={cs.CV}
}
```
