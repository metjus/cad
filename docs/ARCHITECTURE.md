# Architecture

Status: Phase 0 design, partially implemented by the proof of concept (PoC).
Items marked **[PoC]** exist in code today; everything else is design.

## 1. Layering

```
┌──────────────────────────────────────────────────────────────────────┐
│ 1. Presentation (Qt Widgets)   app/                                  │
│    MainWindow, panels, viewport widget, gizmos, command palette      │
├──────────────────────────────────────────────────────────────────────┤
│ 2. Application commands        (Phase 1: commands/)                  │
│    CommandRegistry, tools (Move, Extrude...), undo stack, selection  │
├──────────────────────────────────────────────────────────────────────┤
│ 3. Document / model            document/                             │
│    Document, ParameterTable, objects, MeshReference, units           │
├──────────────────────────────────────────────────────────────────────┤
│ 4. Parametric feature engine   document/ (→ features/ in Phase 2)    │
│    Feature, dependency graph, recompute, rollback, suppression       │
├───────────────────────────────┬──────────────────────────────────────┤
│ 5. Geometry kernel facade     │ 6. NURBS & reconstruction            │
│    kernel/ (OCCT wrappers,    │    kernel/Nurbs (PoC), recon/ (Ph.6) │
│    NamedShape, primitives)    │    mesh/ (import + analysis)         │
├───────────────────────────────┴──────────────────────────────────────┤
│ 7. Persistence (Phase 2: io/)  native .hcad, STEP/IGES/STL/OBJ/3MF   │
├──────────────────────────────────────────────────────────────────────┤
│ 8. Update infrastructure (Phase 1: updater/) Velopack integration    │
└──────────────────────────────────────────────────────────────────────┘
          core/ (units, expressions, ids, errors, logging) used by all
```

Rules (enforced by CMake target dependencies) **[PoC]**:

- `hcad_core` and `hcad_mesh` depend on nothing (no OCCT, no Qt).
- `hcad_kernel` depends on OCCT modelling toolkits only — never on Qt or
  visualization toolkits.
- `hcad_document` depends on `hcad_kernel`; it never includes Qt.
- `hcad_poc` (the app) is the only target linking Qt and OCCT visualization
  (`TKV3d`, `TKOpenGl`). The UI never calls modelling algorithms directly; it
  edits parameters/features and asks the document to recompute.

This keeps the geometry stack testable headless (all 32 unit/geometry tests run
without a display) and makes a future CLI/batch mode or scripting API trivial.

## 2. Document model

```
Document
 ├─ Settings: units (mm|in), tolerances, display precision
 ├─ ParameterTable            "Width = 60 mm", "Height = Width * 0.5"   [PoC]
 ├─ Bodies / Objects          browser organisation (Solids, Surfaces,
 │                            Curves, Sketches, Mesh References,
 │                            Components, Construction, Measurements)
 ├─ FeatureHistory            ordered list + dependency DAG           [PoC: list]
 │   └─ Feature
 │       ├─ id (stable, unique), type, display name
 │       ├─ params: FeatureParam{key, expression, evaluated value, kind} [PoC]
 │       ├─ inputs: feature ids (bodies consumed)                   [PoC: 1 input]
 │       ├─ references: persistent sub-shape references             [PoC]
 │       ├─ state: ok | error(message) | suppressed | rolled back    [PoC]
 │       └─ result: NamedShape (+ NURBS definition for surfaces)    [PoC]
 ├─ MeshReferences            imported polygons + analysis report     [PoC]
 └─ Assemblies (Phase 8)      components, instances, joints
```

Key points:

- **Expressions are authoritative**, evaluated values are caches. Every
  numeric input (feature parameter, sketch dimension, control point offset) is
  an expression that may reference named parameters. Units are parsed in
  expressions (`10 mm`, `2 in`) and stored internally in millimetres. **[PoC]**
- **Parameter evaluation** detects unknown names and cycles and reports them on
  the parameter and on every feature that uses it. **[PoC]**
- **Features never store final geometry as their identity.** They store
  intent (parameters + references). Geometry results are caches that can be
  rebuilt (and are persisted only to speed up file open).
- **Mesh references** are a separate object kind. Importing never creates CAD
  features. Reconstruction (Phase 6) creates new features that *reference* the
  mesh; the mesh itself stays unchanged. **[PoC]**

### 2.1 Feature types (planned)

| Group | Features |
|---|---|
| Primitives | Box **[PoC]**, Cylinder, Sphere, Cone, Torus, Plane |
| Sketch-based | Sketch, Extrude, Revolve, Sweep, Loft |
| Modifiers | Fillet **[PoC]**, Chamfer **[PoC]**, Shell, Draft, Face Offset, Boolean |
| Direct edits | Move Face/Edge/Vertex (translated into Offset/Move/Draft/Replace-face operations) |
| Surfaces | NURBS Surface **[PoC]**, Control Point Edit **[PoC]**, Loft/Sweep/Patch/Bridge, Trim, Extend, Join, Offset, Rebuild, Match |
| Reconstruction | Mesh Analysis, Cleanup, Recognition, Surface Fit, Patch Layout, Stitch, Solidify |
| Patterns | Linear, Circular, Mirror |

### 2.2 Direct modelling as features

A direct manipulation (select face → G → 10 mm) is translated by the tool layer
into a feature with parameters and persistent references, e.g.
`MoveFace{faces=[Box1.Top], vector=(0,0,"10 mm")}` implemented with
`BRepOffsetAPI_MakeOffset`/`BRepAlgoAPI_Defeaturing`/local operations depending
on topology. If an edit cannot be represented as a valid CAD operation, the
tool refuses with an explanation instead of producing a mesh edit.

## 3. Recompute engine

**[PoC]** Linear recompute in history order with rollback index and
suppression; failures propagate to dependants with a message; timing per feature.

**Design for Phase 2–5:**

- Dependency DAG built from (a) feature inputs, (b) sub-shape references, (c)
  parameter identifiers referenced by expressions (`referencedIdentifiers()`
  exists in core **[PoC]**).
- Dirty propagation: editing a parameter marks dependants dirty; only dirty
  features recompute; clean results are reused.
- Background recompute: the document is snapshotted (features are cheap value
  objects; OCCT shapes are immutable handles), computation runs on a worker
  thread with `Message_ProgressRange` for progress/cancel, results are swapped
  in on the GUI thread. The UI stays responsive and shows "Recomputing…".
- Failure policy: a failing feature keeps its last good result for display
  (greyed, flagged) but downstream features are marked as blocked; nothing is
  silently skipped.

## 4. Topological reference stability (Topological Naming Problem)

Implemented core in `kernel/NamedShape` **[PoC]**, tested in
`tests/test_topo_naming.cpp`.

1. **Generator-role names for faces.** The creating feature names each face by
   its role: `Box1.Top`, `Extrude1.Side(Sketch1.Line3)`, `Extrude1.Cap.End`.
   Never by index.
2. **History propagation.** After each OCCT operation the names are carried to
   the result through `BRepBuilderAPI_MakeShape::Modified / Generated /
   IsDeleted`. Faces generated from edges get names derived from the edge
   (`Chamfer1.Edge(Box1.Front|Box1.Top)`). Split faces get piece suffixes
   (`#1`, `#2`). **[PoC]**
3. **Edges and vertices are derived names**: the sorted set of adjacent named
   faces — `Edge(Box1.Front|Box1.Top)`,
   `Vertex(Box1.Bottom|Box1.Front|Box1.Left)`. They survive any upstream change
   that preserves the adjacent faces. **[PoC]**
4. **Ambiguity is an error, not a guess.** If a reference resolves to zero or
   several elements, the feature fails with a clear message. **[PoC]**
5. **Planned extensions (Phase 4–5):**
   - Disambiguation for edges between the same two faces (seams, split edges)
     using ordering along a named neighbour + geometric signature.
   - Geometric fallback matcher: if the semantic name fails, propose the
     candidate with the closest geometric signature (type, centroid, normal,
     area/length) and ask the user to confirm ("repair reference" UI).
   - Sketch-element names: extruded side faces take the name of the sketch
     curve that generated them, sketch curves have stable ids independent of
     solver order.
   - Boolean results: faces keep the name from the operand they came from.

Verified today: a chamfer referencing `Edge(Box1.Front|Box1.Top)` stays on the
same edge while the box width changes 60 → 120 → 25 → 300 mm, OCCT rebuilds all
topology on each recompute, and the UI selection `Box1.Top` is re-selected after
a parameter edit (UI smoke test).

## 5. Units and tolerances

- Internal length unit: millimetre; angles in degrees in the document, radians
  in the kernel call.
- OCCT `Precision::Confusion()` = 1e-7 model units (here 1e-7 mm) is the
  geometric coincidence tolerance; dimensions below it are rejected **[PoC]**.
- A document-level *modelling tolerance* (default 1e-3 mm) controls sewing,
  fitting and recognition; *display tolerance* controls tessellation.
- Imported meshes have no units: the import asks for them **[PoC UI]**.
- Operations report tolerance problems explicitly (e.g. chamfer larger than
  adjacent face). `ShapeFix`/`BRepCheck_Analyzer` validate results **[PoC: check]**.

## 6. Selection model

Viewport selection produces `{bodyFeatureId, persistentReference}` pairs
**[PoC]** (e.g. `Fillet1 : Box1.Top`). Tools and the properties panel work with
these references, never with raw `TopoDS_Shape` pointers, so selections survive
recompute (`SceneSync::restoreSelection`) **[PoC]**. The owning feature of a
selected element is derived from its name, which drives the context-sensitive
properties panel **[PoC]**.

## 7. Commands and tools (Phase 1–2)

- `CommandRegistry`: id, name, description, shortcut, context predicate,
  icon, handler. Menus, toolbar, command palette and tooltips are generated
  from it, which guarantees no orphan UI and makes "do not show unimplemented
  tools" enforceable.
- Modal tools (Move, Extrude…) are state machines receiving viewport events,
  numeric input and axis constraints, and producing a preview feature that is
  committed as one undoable command.
- Undo/redo: command pattern over document changes (feature add/remove/edit,
  parameter edit). Geometry is recomputed, not snapshotted.

## 8. Persistence (Phase 2)

Native `.hcad` = ZIP container:

| Entry | Content |
|---|---|
| `manifest.json` | format version, app version, units, tolerances |
| `document.json` | parameters, features (type, params, references, inputs, state), object tree |
| `cache/<featureId>.brep` | optional OCCT binary BRep caches for fast open (rebuilt if missing or stale) |
| `meshes/<id>.bin` | reference meshes (binary, lossless) |
| `thumbnail.png` | preview |

Versioned schema with forward migrations; unknown feature types are preserved
and shown as "unsupported" rather than dropped. Autosave and crash recovery
write to `%LOCALAPPDATA%/<App>/recovery`. We do **not** adopt OCAF as the
document model (tied to OCCT's TDF, hard to version/diff, poor fit for our
reference scheme); XCAF is used only for STEP/IGES exchange with names/colours.

## 9. Viewport

- OCCT `V3d`/`AIS` in a native child window of a Qt widget **[PoC]**.
- Display objects are rebuilt from the document after recompute (`SceneSync`)
  **[PoC]**; Phase 1 switches to incremental updates keyed by feature id.
- Selection modes Object/Vertex/Edge/Face via AIS selection modes **[PoC]**.
- NURBS control cage and points as non-selectable helper presentations
  **[PoC]**; Phase 7 makes points selectable with a dedicated owner type.
- Mesh references displayed with `AIS_Triangulation` in a distinct colour, not
  CAD-selectable **[PoC]**.
- Phase 1 evaluates moving to OCCT's `QOpenGLWidget` integration (render into
  a Qt-managed FBO) to allow Qt overlays (HUD, gizmo labels) and avoid native
  window quirks; the native-window approach is kept as fallback.

## 10. Update infrastructure (Phase 1)

See TECH_DECISIONS ADR-007. Application binaries live in a per-user
install directory managed by Velopack; user data lives under
`%APPDATA%/<App>` (settings) and the user's Documents (projects). The updater
never touches either. Update check runs in the background, downloads delta
packages, verifies hashes (and Authenticode signature once signing exists),
applies on restart and keeps the previous version for rollback.

## 11. Error handling and logging

- Kernel calls are wrapped: OCCT `Standard_Failure` is caught at the facade and
  converted into a result with a message **[PoC]**.
- No exceptions cross layer boundaries; results carry `error` strings
  **[PoC]**; Phase 1 adds structured error codes.
- Logging: spdlog-style rotating file log in `%LOCALAPPDATA%/<App>/logs`,
  crash handler writes a minidump and the last log lines.
