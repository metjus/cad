# NURBS Architecture

NURBS are first-class geometry from Phase 0, not a later add-on. Mesh
reconstruction (Phase 6) and surfacing (Phase 7) both depend on this design.

## 1. Representation

| Layer | Type | Role |
|---|---|---|
| Document | `NurbsSurfaceDef` **[PoC]**: degreeU/V, countU/V, poles (row-major), weights (empty = non-rational); Phase 7 adds explicit knots + multiplicities, periodicity, trimming loops | Editable, serialisable source of truth |
| Kernel | OCCT `Geom_BSplineSurface` / `Geom_BSplineCurve` | Evaluation, derivatives, knot insertion, degree elevation, continuity |
| Topology | `TopoDS_Face` built with `BRepBuilderAPI_MakeFace` (natural bounds or trimming wires) **[PoC]** | Enables sewing, booleans, fillets, STEP export |
| Display | AIS shape + control cage (polyline compound) + control point cloud **[PoC]** | Cage/points are helper presentations, never authoritative |

The OCCT surface is **always rebuilt from the definition**, so the document
never depends on in-memory kernel objects and can be saved/diffed.

Knot vectors: PoC uses clamped uniform knots (endpoint interpolation, verified
by test `MovingAControlPointDeformsSurfaceLocally`). Phase 7 stores knots
explicitly so that knot insertion/removal and non-uniform rebuilds are
preserved.

## 2. Editability as history

Control-point edits are features, not mutations **[PoC]**:

```
Surface1  NURBS Surface   (SizeU, SizeV, CountU, CountV, Degree, Origin)
CPEdit1   Move Control Point [2,1]  DZ = "15 mm", Weight = "1"
CPEdit2   Move Control Point [2,2]  DZ = "Lift"
```

- Offsets are expressions, so they can be driven by named parameters
  (tested: changing `Lift` moves the pole).
- If an upstream change removes the referenced control point (e.g. fewer poles),
  the edit fails with "Control point [u,v] no longer exists" (tested), it does
  not silently apply elsewhere.
- Face names are carried through edits (`Surface1.Surface`), so downstream
  references (trim, thicken, stitch) remain valid (tested).

Phase 7 extends this with group edits (`MoveControlPoints{indices, transform}`),
`InsertKnot`, `ElevateDegree`, `Rebuild{degree, spans, tolerance}`,
`SetWeight`. Interactive gizmo drags are coalesced into one feature per drag.
Control point indices are stable under knot insertion by mapping old→new
indices in the feature that changes the structure.

## 3. Operations and their OCCT building blocks

| Capability | OCCT tools | Phase |
|---|---|---|
| Curves through points / control point curves | `GeomAPI_Interpolate`, `GeomAPI_PointsToBSpline`, direct `Geom_BSplineCurve` | 7 (sketch splines in 3) |
| Rebuild / approximate | `GeomConvert_ApproxSurface`, `GeomAPI_PointsToBSplineSurface`, `ShapeCustom` | 6/7 |
| Knot insert/remove, degree elevation | `Geom_BSplineSurface::InsertUKnot`, `RemoveUKnot`, `IncreaseDegree` | 7 |
| Loft / sweep | `BRepOffsetAPI_ThruSections`, `BRepOffsetAPI_MakePipeShell`, `GeomFill_*` | 4/7 |
| Patch (N-sided), constrained surface | `BRepFill_Filling`, `GeomPlate_BuildPlateSurface` | 7 |
| Bridge / blend surface | `GeomFill_ConstrainedFilling`, custom G1/G2 bridge from boundary derivatives | 7 |
| Trim / split | `BRepAlgoAPI_Splitter`, `BRepFeat_SplitShape`, trimming wires on faces | 7 |
| Extend | `GeomLib::ExtendSurfByLength` | 7 |
| Join / sew | `BRepBuilderAPI_Sewing`, `ShapeFix_Shell` | 6/7 |
| Offset / thicken | `BRepOffsetAPI_MakeOffsetShape`, `BRepOffsetAPI_MakeThickSolid` | 7 |
| Solidify closed shells | `BRepBuilderAPI_MakeSolid`, `ShapeFix_Solid` | 6/7 |
| Match / align to edge | custom: adjust boundary rows of poles for G0/G1/G2 using derivatives of the target | 7 |

## 4. Continuity

- G0: boundary curves coincide within tolerance.
- G1: tangent planes match (normal angle below angular tolerance).
- G2: curvature matches (normal curvature along the boundary).

Measurement: sample along shared edges, evaluate with `GeomLProp_SLProps` /
`LocalAnalysis_SurfaceContinuity`, report max deviation per criterion and
colour the edge (green/yellow/red). Construction for matching: for a
clamped B-spline the first 1/2/3 rows of poles control G0/G1/G2 at a boundary,
which is how "Match" tools adjust surfaces.

Analysis displays (Phase 7): curvature map (Gaussian/mean/min/max) via per-vertex
values on the display tessellation, zebra stripes (environment-reflection
shader in OCCT `Graphic3d_ShaderProgram`), curvature combs for curves and
surface iso-curves, deviation map against reference meshes.

## 5. Surface-to-solid

Sheets are sewn (`BRepBuilderAPI_Sewing` with the document modelling
tolerance), healed (`ShapeFix`), checked (`BRepCheck_Analyzer`), and if closed,
converted to a solid. Open boundaries are reported with their location. The
resulting solid is a normal B-Rep usable by booleans, fillets and STEP export.

## 6. Limitations (current)

- PoC surfaces are untrimmed and created from a planar grid; editing is by
  feature parameters in the properties panel (no gizmo yet).
- Control points are displayed but not individually selectable yet.
- No explicit knot editing in the PoC (uniform clamped knots).
