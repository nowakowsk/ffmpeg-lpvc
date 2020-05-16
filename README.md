Original FFmpeg's `README.md` has been moved to `FFmpeg.README.md`.

# FFmpeg with Longplay Video Codec support

This repository contains mirror of FFmpeg source code with added support for Longplay Video Codec (LPVC).

To review what has been added compare branch `lpvc` to `master`.

# Build

## Dependencies
- Zstandard
- liblpvc

## Configuration

Add following parameters to configuration command:

`./configure --ld=g++ --enable-zstd --enable-liblpvc [...]`