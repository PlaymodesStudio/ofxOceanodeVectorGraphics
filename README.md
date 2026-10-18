# ofxOceanodeVectorGraphics

## Body Repulsion

Available under **Vector Graphics → Body Repulsion**. Body points retain their
positions and velocities as Force points push them away, with smooth repulsion
inside each point's fear radius. A spring pulls each point toward its current
incoming Body position, and damping settles the motion. All inputs and controls
are sampled on every node update, so momentum continues without input changes.

| Parameter | Meaning |
| --- | --- |
| `Body_X`, `Body_Y` | Paired vectors of moving spring rest positions. |
| `Force_X`, `Force_Y` | Paired vectors of repeller positions; each Body point reacts to all of them. |
| `Strength` | Per-Force repulsion multiplier, default 1. Zero disables a repeller, 0.5 halves its force, and values above 1 amplify it. One value broadcasts; shorter vectors repeat their last value, excluding Force separators. Empty, negative or nonfinite values disable the corresponding repulsion. |
| `FearDistance` | Radius in coordinate units; default 0.15. One value broadcasts. Multiple values map to Body points, excluding separators; a shorter vector repeats its last value. Empty, negative or nonfinite fear values disable repulsion for the corresponding points. |
| `Repulsion Strength` | Per-Body push strength, default 40. Higher values produce a faster escape. |
| `Return Strength` | Per-Body spring stiffness, default 8. Lower values give a looser shape; zero allows free drift. |
| `Damping` | Per-Body momentum decay, default 3. Lower values allow longer drifting and more spring oscillation; zero removes drag. |
| `Reset` | Restore current Body coordinates and clear momentum on the next update. |
| `Out_X`, `Out_Y` | Paired simulated positions, with no canvas clamping. |

All vector controls accept a float connection or a one-value vector: that value
applies to every corresponding point, preserving the original scalar behavior.
FearDistance, Repulsion Strength, Return Strength and Damping map to Body points;
Strength maps to Force points. Path separators do not consume control values.
Shorter control vectors repeat their last value; excess values are ignored.
Empty, negative or nonfinite control values act as zero. Repulsion Strength and
Return Strength are bounded to 0–1000 per point, and Damping to 0–100. Changing
any of these vectors during motion preserves existing positions and velocities.

For a gentle floating response, start with Repulsion Strength 40, Return Strength
4 and Damping 2. For a quicker, calmer recovery, try Return Strength 12 and
Damping 7. Both coordinate sets must use the same space. With normalized 0–1
coordinates, FearDistance 0.15 is 15% of one coordinate axis; use equally scaled
X and Y coordinates for circular influence in the rendered image.

Repulsion is evaluated at the **simulated** position, with a squared falloff to
zero at the fear boundary. Each contribution is multiplied by its Force point's
Strength and the Body point's Repulsion Strength. Control changes take effect on the
next update, preserving existing momentum. Multiple forces add; opposing contributions can
cancel. Points have unit mass and move independently; this does not implement
point-to-point collisions or a connected soft-body mesh. Incoming Body changes
move the spring targets smoothly rather than overwriting simulated positions.
At exact overlap a deterministic escape direction avoids undefined normalization;
identical points with identical fear values remain together.

Paired `-1` values are preserved as path separators and never simulated. The
coordinate value `-1` is reserved by the addon's path protocol. Empty, mismatched,
nonfinite, or incorrectly separated Body input clears outputs and simulation
state. Mismatched Force vectors provide no repulsion; invalid Force points are
ignored while spring motion and inertia continue. State is tracked by input
index. A point count or separator-layout change resets the whole simulation;
reordering equally sized vectors changes the spring targets at those indices.
Loading presets and resetting Oceanode's global phase also reset momentum.

The CPU implementation uses fixed 1/240-second substeps and processes at most
0.1 seconds of host time per update, dropping excess time after a long stall.
It checks every Body/Force pair per substep; large point sets may need a future
spatial index or GPU backend. Only controls and inputs persist in presets,
not simulated velocities. When either coordinate output changes, both vectors
are staged before events and Y is notified before X.

Run `python3 tests/body_repulsion_regression.py` on macOS with compiled sibling
openFrameworks libraries, or add `--core-only` for standalone physics checks.
The runtime adapter uses real OF parameters/events and Oceanode float-to-vector
connections, with a supplied frame clock, simulated node registration and unused
GUI teardown. Visual tuning still needs a rebuilt host app.

## Trim Paths Pro

Available under **Vector Graphics → Trim Paths Pro**. Combines local edge trimming,
per-path reveals, whole-input sequencing and overlapping visual groups. Start,
End and Origin map independently; Origin shifts the trim with wrapping, and
progress can use edge count or distance. Existing trim nodes remain available
for older patches.

See [Trim Paths Pro usage and vector mapping](docs/trim_paths_pro.md) for examples,
output formats, group editing and legacy equivalents. Run
`python3 tests/trim_paths_pro_regression.py` on macOS with compiled sibling OF
libraries, or add `--core-only` for the standalone geometry regressions.

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
