# ofxOceanodeVectorGraphics

## SVG Polyline Loader

Available under **Vector Graphics → SVG Polyline Loader**. The existing SVG
Modulator remains available for older patches.

- **Load SVG** opens the native file picker in the application's `data` folder.
  Cancelling leaves the current file unchanged; selecting the same file reloads it.
- **File** accepts an absolute path or a path relative to `data`, and is saved in
  presets. The SVG is referenced externally, not embedded in the preset.
- **Curve Resolution** (default 32, range 2–1024) sets the number of segments per
  Bezier curve. Increasing it improves curved outlines. Straight segments retain
  their vertices; this is not uniform distance resampling.
- **Normalize** defaults to on: divide X by the SVG viewport width and Y by its
  height. Disable it for original viewport coordinates. Coordinates outside the
  viewport remain outside 0–1, and the Y axis follows SVG (downwards).
- **Rotate90º** rotates clockwise by 90° per click; four clicks restore the original
  orientation. Rotation is around (0.5, 0.5) when normalized, or the SVG viewport
  center otherwise. The viewport stays fixed, so rotating a non-square SVG in
  original coordinates can put vertices outside its original viewport.
- **FlipX** mirrors horizontally and **FlipY** vertically about the same center,
  after rotation. Separators are never transformed. These settings survive file
  reloads and sampling changes; flips and the Inspector's **Quarter Turns** value
  are saved in presets. Set Quarter Turns to 0 to reset rotation.
- **Scale** uses 0–1, default 1 (original size); 0 collapses every contour to the
  canvas center. Scaling is uniform about the same center as rotation.
- **Rotate** uses −1…1 for −90°…90°, default 0. Positive angles turn clockwise.
  This angle adds to the rotation applied with Rotate90º.
- **MoveX / MoveY** use −1…1, default 0. Values are fractions of the full canvas
  width/height; positive moves right/down. With Normalize off, those offsets are
  multiplied by the SVG width/height. Translation is applied after rotation,
  flipping and scaling. Outputs are not clamped to the canvas.
- All transform controls persist in presets and are reapplied after file reloads
  and sampling changes.
- **Vertex_Y**, then **Vertex_X**, contain equally sized float vectors. Y is
  published before X, matching SVG Modulator, Points Resampler and Shape Transform.
  Each nonempty contour ends in `-1` in both vectors, including the last contour.
  Closed contours repeat their first vertex before that separator. The `-1`
  value is reserved by this coordinate protocol.

The loader uses the existing `ofxSvg` dependency and its supported SVG subset.
It preserves open subpaths and fill-only outlines, flattens Beziers, handles
transforms and viewBox-only exports, and keeps the parser's path order. Text,
embedded images, clipping, CSS and other advanced SVG features are limited by
`ofxSvg`; convert text to outlines when exporting. Color output is not included.
Invalid or missing files clear the coordinates and show an error on the node.

Run `python3 tests/svg_polyline_loader_regression.py` on macOS with compiled
sibling openFrameworks libraries. It syntax-checks the addon and runs real SVG
parsing/geometry tests with ASan/UBSan; node registration and the native dialog
are simulated in the runtime fixture. The native UI still needs a host-app check.
