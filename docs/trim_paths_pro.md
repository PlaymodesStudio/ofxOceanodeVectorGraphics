# Trim Paths Pro

Find the node in **Vector Graphics → Trim Paths Pro**. It defaults to trimming
each path by distance, with Start 0, End 1, Origin 0, and connected output.

## Scope and vector mapping

A path is a nonempty run of vertices between paired `-1` separators. An edge is
the line between two consecutive vertices in one path. For paths with 4, 2 and 5
vertices, there are **3 paths and 8 edges**, irrespective of separators.

| Scope | Start and End describe | Available mappings |
| --- | --- | --- |
| Edges | A local interval on every edge | Scalar, Per Path, Per Edge |
| Paths | An interval along each complete path | Scalar, Per Path |
| All Paths | One interval across the entire input | Scalar |
| Groups | An interval across each selected group | Scalar, Per Group |

**Start Map, End Map and Origin Map are independent.** All three use the mappings
available for the selected scope. Auto broadcasts a one-element vector;
otherwise its size must identify exactly one supported mapping. An ambiguous
size produces `Valid = false` and an explanation in Status. Choose an explicit
mapping to resolve ambiguity. Explicit Start/End/Origin mappings require the exact
domain size; use Auto or Scalar for broadcasting.

For example, with three paths:

- Paths, Start `{0}`, End `{0.2, 0.5, 0.8}` reveals different portions of the three
  paths. Longer paths may have different numbers of vertices.
- Edges, Start with 3 values and End with 8 values applies each path's Start to
  all its edges, with a separate End for every edge.
- Groups, Start `{0}`, End `{0.3, 0.7}` independently reveals two selected groups.
- All Paths accepts scalar Start and End; additional elements are an error.

Per-edge indices are global across the input, in path/vertex order. Per-path
indices include singleton paths, although singletons have no edges to trim.
Vectors count **edges or paths, never raw X/Y entries or vertices**. No vector
cycling, truncation or implicit resizing occurs when the geometry changes.

## Progress, range and output topology

- **Origin:** normalized offset from 0 to 1, added to Start and End together.
  The interval wraps at the end, preserving its length. Origin 0 and 1 are
  equivalent. Start 0, End 0.25, Origin 0.9 shows 0.9–1 followed by 0–0.15.
  Origin 0.5 starts halfway along the selected edge, path, input, or group.
  It changes traversal order without changing the original source IDs.
- **Edge Count:** every edge gets an equal share of progress, regardless of length.
- **Distance:** progress follows cumulative geometric length; gaps between paths
  contribute no distance. Zero-length edges contribute no distance.
- **Keep Order off:** clamp both bounds to `[0,1]`, then swap them if reversed.
- **Keep Order on:** a reversed interval is empty. Equal bounds are always empty.
  These rules apply to Start/End before Origin is added, so crossing the seam
  does not make a valid shifted interval empty.
- **Connected Paths:** reconnect consecutive visible edges only when their
  original shared endpoint is fully retained. Never bridge trimmed gaps, paths,
  or different groups.
- **Separate Edges:** every visible edge gets its own separator.
- **Endpoint Dots:** append singleton paths at visible edge endpoints, including
  cut endpoints and interior vertices. Deduplicate identical positions within a
  source path/group.

Edges scope always trims locally; its Progress selector does not change the
result. Zero-length edges have zero completeness and emit no geometry. In Edge
Count sequences they still occupy a progress slot. Singletons emit no geometry.
Closed paths must repeat their first vertex before the separator; no closing edge
is added implicitly. Wrapped output stays connected across the original seam
only when it is a closed path. On open paths, the tail and head remain separate.
For All Paths and Groups, wrapping cycles through the entire sequence and keeps
breaks between distinct source paths.

Origin Map follows the same vector rules as Start Map and End Map. In Paths
scope, scalar Start/End with Origin `{0, 0.25, 0.5}` moves each of three paths'
trim windows independently. In Edges scope it can map per path or per edge;
in Groups scope it can map per group. Full Start 0 / End 1 still draws a full turn
when Origin moves, with the compact output starting at the shifted origin.

## Groups

Enable **Show GUI**. Choose a group, then click paths in the preview or use the
Path membership checkboxes. The preview fits the input bounds and identifies
paths by index. Select All Paths and Clear Group edit the focused group. Num
Groups is also available in the Inspector (1–100).

Groups initially contain no paths. Membership can overlap: a path in two groups
produces independent fragments with separate separators. Unselected
paths are omitted. Traversal and output use group order, then input path order.
Group membership is stored in presets, including empty groups. Indices refer to
the input path ordering, not persistent shape identities. If an index temporarily
exceeds the path count, it is skipped with a Status warning and retained in the
saved selection so it can become active again when that path returns.

## Outputs and validation

| Output | Layout |
| --- | --- |
| Out.X / Out.Y | Compact visible geometry, with matching `-1` separators and a trailing separator |
| Completeness | Total visible local fraction of each original edge, including both pieces if split by wrapping; zero for invisible or zero-length edges |
| FullSegment.X / FullSegment.Y | First visible piece in traversal order for each stable edge slot: start, end, `-1`; invisible slots are `0,0,-1` |
| WrappedSegment.X / WrappedSegment.Y | Second visible piece if the same edge is cut twice by a wrapped interval; otherwise `0,0,-1`, with the same stable slots as FullSegment |
| Source.Path / Source.Edge / Source.Group | One zero-based source ID per stable slot; Group is `-1` outside Groups scope |
| Path Count / Edge Count | Counts in the parsed input, before grouping |
| Group Count | Scalar number of configured groups (including empty groups), available in every scope |
| Valid / Status | Validation result and resolved Start/End/Origin mappings, or a diagnostic |

Stable slots are in original edge order outside Groups scope. Inside Groups they
are in group/path/edge order, with a slot for every selected group/edge occurrence;
overlapping groups intentionally repeat Source.Edge IDs. Endpoint dots add no
stable slots. To draw FullSegment geometry, use Completeness to mask invisible
slots; their zero coordinates alone do not indicate visibility. If an edge splits
at the seam, also draw its WrappedSegment slot. Wrapped slots without a second
piece contain the collapsed placeholder `0,0,-1`. Out.X/Y already includes both
pieces with the correct separators. Compact output indices must not be confused
with stable-slot indices.

Coordinates must be finite, X/Y sizes must match, and separators must align.
Leading, repeated and trailing separator pairs are allowed. The legacy XY
protocol reserves `-1` in either coordinate; a cut landing exactly on this value
is rejected rather than emitting an accidental separator. Transform such input
coordinates before trimming.

Empty, nonfinite, ambiguous or incorrectly sized controls invalidate the result.
Invalid results clear visible geometry. If the geometry and scope
could be parsed, stable slots remain present with zero completeness and valid
separators. Malformed geometry has no stable slots.

All processing inputs mark the node dirty. Changes are coalesced until its next
`update`, so moving End or Y works even when X is static. Depending on graph update
order, this can introduce one update cycle of latency. Every output is staged
before notifications; metadata and Y notify before X, following the addon’s
X-trigger convention. Feedback changes received during publication are processed
on the following update, without recursive recalculation.

## Legacy equivalents

| Existing node | Trim Paths Pro settings |
| --- | --- |
| Trim Path | Edges, Separate Edges; choose independent scalar/path/edge mappings |
| Trim Path Sequential, Sequential off | Edges, Separate Edges |
| Trim Path Sequential, Sequential on | All Paths, Edge Count, Separate Edges, scalar Start/End |
| Trim Group Paths | Groups, Edge Count, Connected Paths, Keep Order on; recreate selections |

Color and opacity processing are not included; apply styling downstream.

These preserve the intended trimming operations while correcting equality, broadcasting,
clamping, separators, group bridges and parameter-update bugs. Existing nodes
and presets are unchanged; legacy groups are not automatically imported.

## Validation

`python3 tests/trim_paths_pro_regression.py` runs deterministic geometry cases,
1,000 randomized geometry/control cases, a full addon header/registration syntax
check, and adapter tests using real OF parameter events, JSON and ImGui. Only
model registration is substituted in the runtime fixture. Runtime tests use
AddressSanitizer and UndefinedBehaviorSanitizer. The ImGui editor is exercised
headlessly; interactive appearance and clicking still need a host-app check.

The full suite expects compiled macOS OF libraries and sibling addons. Set
`OF_ROOT` for another checkout. `--core-only` runs the portable C++ geometry suite
without OF dependencies.
