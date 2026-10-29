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

## TwoBody Repulsion

Available under **Vector Graphics → TwoBody Repulsion**. Two bodies, A and B,
repel each other with the same inertial spring model as Body Repulsion, each
being the other's repeller. Every point is a disk of its body's Radius, and the
gap between disk surfaces drives the push, so with Radius 0 and a passive B the
node reproduces Body Repulsion with B's points as Force points exactly. A
border wall keeps both bodies inside the 0–1 canvas.

| Parameter | Meaning |
| --- | --- |
| `A Body_X`, `A Body_Y`, `B Body_X`, `B Body_Y` | Paired vectors of moving spring rest positions, one pair per body, with `-1` path separators. |
| `A Radius`, `B Radius` | Disk radius of each point, default 0. The interaction distance is measured between disk surfaces, and the border is measured to the disk edge. |
| `A Strength`, `B Strength` | How strongly this body pushes the other, default 1. Zero makes the body invisible to the other; 0.5 halves its push. |
| `A FearDistance`, `B FearDistance` | Surface gap at which this body starts fleeing, default 0.15. Zero disables the response, even inside an overlap. |
| `A Repulsion Strength`, `B Repulsion Strength` | Per-point push response, default 40. Also scales that body's border push. |
| `A Return Strength`, `B Return Strength` | Spring stiffness toward the incoming coordinates, default 8; zero allows free drift. |
| `A Damping`, `B Damping` | Momentum decay, default 3; zero removes drag. |
| `Border Margin` | Distance from the canvas edge that no disk may cross, default 0. A hard limit: positions are clamped and momentum into the wall is dropped. |
| `Border Feather` | Width of a soft push inside the margin, default 0. The push ramps with a squared falloff over the feather width and is full strength past the margin. |
| `Reset` | Restore both bodies to their current coordinates and clear momentum on the next update. |
| `A Out_X`, `A Out_Y`, `B Out_X`, `B Out_Y` | Paired simulated positions per body. |

All per-body controls map to that body's points, excluding separators; one
value broadcasts, shorter vectors repeat their last value, and float
connections work as single values. Empty, negative or nonfinite control values
act as zero. For each A/B pair the surface gap is the centre distance minus
both radii; A accelerates away by `A Repulsion Strength × B Strength ×
A FearDistance × falloff²` while the gap is inside `A FearDistance`, and B does
the same with its own fear and strength roles swapped. Inside an overlap the
push is constant, and exactly coincident points part in opposite directions.
Each body keeps its own identity: a point count or separator layout change
resets that body only, and an empty or invalid body clears its own output while
the other keeps simulating.

The border is active when `Border Margin` or `Border Feather` is above zero and
assumes normalized 0–1 coordinates; with both at zero the output is unclamped,
like Body Repulsion. A disk too large to fit is held at the canvas centre.

Both bodies share one clock with fixed 1/240-second substeps and at most 0.1
seconds of host time per update. Every A/B pair is evaluated once per substep
and pushes both sides, pairs outside the largest reach skip the square root,
and bodies whose expanded bounding boxes do not overlap skip the pair loop
entirely. On an Apple Silicon laptop two bodies of 300 points cost about 0.8 ms
per 60 fps frame while overlapping and under 0.03 ms while apart; 1000 points
per body cost about 8 ms while overlapping, so a spatial index would only pay
off beyond roughly 500 points per body.

Run `python3 tests/two_body_repulsion_regression.py` on macOS with compiled
sibling openFrameworks libraries, or add `--core-only` for standalone physics
checks. The node tests share `tests/support/oceanodeHeadlessStubs.h` with Body
Repulsion, which stands in for the GUI scope, scheduling and dropdown graph
dependencies of core parameters.

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

## Wave It

Available under **Vector Graphics → Wave It**. Deforms any XY contour along its
local perpendicular, including MetaballsV3, SVG outlines and generated paths.

| Parameter | Meaning |
| --- | --- |
| `In.X`, `In.Y` | Equally sized coordinate vectors, with paired `-1` path separators. |
| `Wave In` | Normalized waveform. `0.5` is neutral, `0` gives negative Amount and `1` gives positive Amount. Default `[0.5]`; a singleton applies a constant offset. |
| `Amount` | Vector of target displacement amplitudes in coordinate units, default `[0.05]`, range −1…1. Negative values invert the wave locally; zero targets the original contour. |
| `Response` | `Direct` (default) follows the target immediately. `Spring` follows with inertia and mild, decaying overshoot. |
| `Stiffness` | Vector of spring strengths, default `[40]`, range 0–1000. Higher values respond faster; lower values move more softly. Zero holds the current normal offset and clears its velocity. Ignored in Direct mode. |
| `Motion Influence` | Vector of input-motion coupling strengths, default `[0]`, range 0–1. Zero preserves the original wave-only behaviour; one gives full normal-direction coupling. Active only in Spring with positive Stiffness. |
| `Out.X`, `Out.Y` | Deformed coordinates with the same vertex count, ordering, separators and closure as the input. No canvas clamping. |

Wave In, Amount, Stiffness and Motion Influence each restart their entire vector for every separate
path, including holes. Each vector may have a different length; single values
broadcast everywhere, and float connections work as single-value vectors. Samples are
linearly interpolated by normalized **distance along the original contour**;
uneven vertex spacing therefore does not compress or stretch local wave cycles.
The geometry itself is not resampled. A 1000-sample sine buffer containing three
cycles produces three ripples around each closed shape, evaluated at its existing
vertices. Use enough input vertices to resolve the waveform; this node does not
add detail beyond the input geometry.

Closed paths are detected by an exactly repeated first/last coordinate. They
sample Wave In as a periodic buffer, including interpolation from the final sample
back to the first. Supply periodic samples without duplicating the wave endpoint
(e.g. `0.5 + 0.5*sin(2*pi*3*i/1000)`, for `i=0…999`). The repeated geometry endpoint
always gets exactly the first point's deformed position. Positive displacement
points outward for simple closed shapes, independent of winding. Each hole is
handled independently, so positive displacement enlarges its outline too.

Open paths retain open endpoints and use the first/last wave samples at their
start/end. Their positive normal is `(-tangent.y, tangent.x)`; reversing the path
reverses that side. Normals use the average of adjacent unit edge directions,
with one-sided normals at open endpoints. Adjacent duplicate vertices share one
position and normal. Isolated points and zero-length paths remain unchanged;
reversing cusps use the outgoing direction. Self-intersecting loops have no
unambiguous inside/outside, and large offsets may create intersections. Wave It
does not repair or recompute contour topology.

Empty Wave In or Amount targets zero deformation. Wave samples are clamped to
0–1; nonfinite samples act as 0.5. Empty/nonfinite Amount, Stiffness and Motion Influence samples
act as zero; finite values are bounded to their displayed ranges. Invalid, nonfinite or mismatched XY
pairs clear both outputs. The coordinate value `-1` is reserved by the addon's
path protocol; avoid deformations that place real coordinates exactly at `-1`.

All input edits are batched for the next node update, including Wave In, Amount,
Stiffness and Motion Influence changes while geometry is stationary. Both outputs are staged before
notifications, with Y notified before X. Direct skips work on clean updates;
Spring advances on every update, even with unchanged inputs.

Spring simulates a **scalar normal displacement** at each vertex; the original
input geometry remains its moving rest shape. The local target is
`(2 * Wave In - 1) * Amount`. Stiffness changes response speed, not the final
amplitude. Springs have unit mass and a fixed damping ratio of 0.65, giving a
small overshoot that settles. There is no neighbour coupling or membrane tension.
Amount is a target amplitude; inertia can temporarily carry displacement beyond it.
With motion excitation disabled or the input stationary, zero Amount or neutral
Wave In returns gradually to the base contour in Spring mode (provided Stiffness
is positive). An Amount envelope can therefore localize
ripples; a Stiffness envelope makes some regions react faster than others.

**Motion Influence** couples movement of the incoming geometry into the spring.
At each vertex, input movement since the last update is projected onto the current
normal. Positive coupling creates lag behind that movement, followed by a damped
rebound when it stops. Tangential movement does not excite the normal spring.
The wave target and motion forcing combine independently: motion works with
Wave In `[0.5]` or Amount `[0]`, and its deformation is not capped by Amount.
This remains a normal-displacement effect, not full XY particle inertia.

Try **Response Spring**, **Stiffness `[40]`**, **Motion Influence `[0.5]`** and a
neutral Wave In, then translate the incoming circle and stop it. Try a Motion
Influence vector such as `[0, 1, 0, 1]` to vary the response around each loop.
Zero Stiffness still holds the current offset and ignores motion. Setting Motion
Influence back to zero stops new excitation; existing spring motion settles.

Motion history is rebased on startup, preset/phase reset, invalid geometry,
topology changes and entry into Spring mode. Input positions are tracked even
with influence zero, so enabling it cannot replay old movement. Invalid, zero,
very short (at most one microsecond) or longer-than-0.1-second frame intervals
also rebase motion without excitation. Ordinary coordinate jumps with stable
vertex layout are treated as motion, including teleports and upstream index
reordering; use a reset or disable influence during intentional discontinuities.

The spring uses an exact damped solution for the target and normal held over each
frame, with input motion treated as constant velocity during that interval.
At most 0.1 seconds of elapsed host time is processed per update; excess time
following a long stall is dropped. Switching to Direct immediately applies the
current target and clears velocity; switching from Direct to Spring starts at
the last Direct displacement. A newly created Spring starts at zero displacement.
Loading a preset or resetting global phase clears all spring motion.

Changing point count, separator layout, closure or adjacent-duplicate layout
resets spring offsets and velocities to zero, then advances the new geometry.
Otherwise state follows vertex indices, including while coordinates move. Equal-size
shape reordering or a changed contour starting point cannot be identified as new
geometry. This is not persistent shape tracking across Metaballs splits/merges.

Connect **MetaballsV3 Contours X/Y → Wave It In.X/In.Y**, then connect a normalized
sine bank to **Wave In** and start with **Amount `[0.03]`**. For an elastic
response, select **Spring** and try **Stiffness `[40]`**; try `[5, 80, 5, 80]`
for alternating soft and fast regions around a loop. Connect Out.X/Out.Y to the
same downstream path generator or renderer that previously received the contours.

Run `python3 tests/wave_it_regression.py` for geometry, spring motion,
frame-rate consistency, motion lag/rebound, vector controls, scalar connections
and real OF event checks
with ASan/UBSan plus addon registration syntax validation. `--core-only` runs the
standalone geometry checks. Visual verification requires rebuilding the host app.

## Wave Pro

Available under **Vector Graphics → Wave Pro**. Ripples any XY contour along its
local normal, and can treat the contour as an elastic membrane, so ripples travel
along the body, ring and settle. It has the same intent as Wave It but its own
physics model.

| Parameter | Scope | Meaning |
| --- | --- | --- |
| `Bypass` | — | Passes the input through unchanged. Toggling it resets the membrane, so the effect restarts at rest on the wave. |
| `In.X`, `In.Y` | — | Contour points, paired `-1` separators between shapes. |
| `Wave In` | per shape | Normalized wave (0.5 neutral) stretched from the start to the end of every shape, resampled by arc length onto its vertices. Closed shapes read it periodically. |
| `Wave Offset` | one value per shape | Scrolls the wave along the shape (1 = a full perimeter). Values cycle across shapes, so `[0, 0.5]` puts alternate shapes in antiphase. |
| `Wave Repeat` | one value per shape | Tiles the wave N times around the shape. Non-integer values leave a seam on closed shapes. |
| `Distribute` | — | `Per Shape` (default): every shape reads the whole Wave In. `By Length`: the `Wave Cycles` contained in Wave In are shared out between shapes in whole cycles, in proportion to their perimeters, so every blob gets a similar wavelength and each loop closes on a whole cycle. Every shape gets at least one cycle when there are enough; with fewer cycles than shapes, the smallest get none and stay flat. All shapes together read Wave In exactly once. Shares are kept from frame to frame (blobs matched by position) until a shape's fair share is 3/4 of a cycle away, so a slowly growing blob does not flicker between two counts; when it does change, the pattern on that blob and its partner changes in one step. Merges and splits recompute the shares. |
| `Wave Cycles` | — | By Length only: how many cycles your Wave In contains, default `1` (e.g. `16` for a 1024-sample buffer drawing 16 sine cycles). Cycle boundaries should fall on whole buffer positions for seamless loops. |
| `Anchor` | — | Where closed shapes start reading the wave; Stable and Tracked always run clockwise on screen, whatever the input's direction. `Tracked` (default): follows the body from frame to frame. Last frame's start point is moved with the shape (centre translation, plus rotation of its long axis when it is elongated) and snapped to the nearest point on the new outline, so it never hops when a worm bends, curls or turns. New shapes start at their right side. `Stable`: recomputed every frame where a +X ray from the shape's centre leaves the outline; this can hop when the body bends so that the ray misses it or crosses it several times. `Path Start`: follow the input order; use it when the start point is meaningful. Open paths always follow the input order. |
| `Resample` | — | Default `512`. Rebuilds every shape with this many evenly spaced points (closed shapes start at the anchor and keep their closing point); `0` keeps the input points. Needed for contours whose points slide along the outline every frame (MetaballsV3): otherwise each wave feature, especially square-wave corners, snaps from point to point and the shape shimmers. It also keeps the point count steady, so Spring state survives. Output point count = Resample (+1 closing point on closed shapes). |
| `Amount` | along perimeter | Displacement for wave 0/1, in coordinate units. |
| `Normal Smooth` | along perimeter | Default `0.05`. Averages normals over a window (fraction of perimeter), which calms spikes at sharp corners and cusps. |
| `Response` | — | `Direct` follows the wave exactly. `Spring` simulates the membrane. |
| `Stiffness` | along perimeter | Pull toward the wave. Higher responds faster. `0` is plastic: nothing pulls back, so dents from motion stay (with Propagation they spread out and even out). |
| `Damping` | along perimeter | Damping ratio. `0` rings forever, `1` settles without overshoot, `>1` is sluggish. It also calms ripples travelling along the outline, more strongly the finer they are. |
| `Propagation` | along perimeter | Ripple speed along the contour, in perimeters per second. Disturbances (from motion or from changing the wave) travel around the body; the wave itself is never flattened. `0` means each point is independent. |
| `Motion Influence` | along perimeter | Inertia of the membrane against the body's own acceleration: it is pressed in while the body speeds up, bulges forward when it brakes, sloshes on turns and bulges outward when it spins. Constant-speed travel causes no deformation. Body motion is measured on the whole shape (centre and long axis), so outline noise is not amplified. |

Every "along perimeter" vector is resampled by arc length on each shape
independently; a single value broadcasts. The membrane is solved implicitly
(Newmark average acceleration with a cyclic tridiagonal solve per shape), so it
is unconditionally stable, does not lose energy numerically, and is substepped
for accuracy at low frame rates. Tension acts on the deviation from the wave, so
the outline rests exactly on it. The membrane starts at rest on the wave, and
restarts there on reset, preset load, Bypass or a change of vertex layout (for
example when metaballs merge or split). As a safety stop, the deviation from the
wave is limited to half the body's size.

Ideas to try: Spring, Stiffness `[5]`, Damping `[0.1]`, Propagation `[0.5]`, then
pulse Amount: the shape rings and waves chase each other around it. Or Motion
Influence `[1]`, Stiffness `[20]`, Damping `[0.3]`, Propagation `[1]` on moving
metaballs, for jelly. Very low Damping with high Propagation makes fast, busy
ripples; Stiffness `0` with strong Motion Influence soon reaches the safety stop.

Run `python3 tests/wave_pro_regression.py` (or `--core-only`).
