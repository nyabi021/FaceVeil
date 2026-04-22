# FaceVeil

FaceVeil is a macOS-first desktop app that scans image files, detects faces with an SCRFD ONNX model, and writes mosaic-protected copies to an output folder.

## Requirements

- macOS
- CMake 3.24+
- Qt 6
- OpenCV 4
- ONNX Runtime

The current Homebrew-friendly setup expects `pkg-config` to find `libonnxruntime`.

## Build

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
```

Run:

```bash
open build/FaceVeil.app
```

Or directly:

```bash
./build/FaceVeil.app/Contents/MacOS/FaceVeil
```

## Model

Download an SCRFD ONNX face detector yourself and select it in the app. A practical default is an SCRFD 2.5G or 10G model exported to ONNX. Place it under `models/` if you want to keep project assets together.

The app expects an SCRFD-style output layout:

- score tensors for strides 8, 16, 32
- bbox distance tensors for strides 8, 16, 32
- optional 5-point landmark tensors

## Notes

- Original images are not overwritten. Processed copies are written to the chosen output folder.
- Increase `Mosaic block size` for stronger anonymization. The UI allows values up to `200`.
- Supported input extensions are `jpg`, `jpeg`, `png`, `bmp`, `tif`, `tiff`, and `webp`.
- HEIC support depends on the local OpenCV build and is not enabled by default here.
