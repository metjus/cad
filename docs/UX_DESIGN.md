# UX Design

Goal: a user immediately knows which tool is active, what is selected, which
mode they are in, what the operation will do, and where to change its
parameters. Inspiration: Blender's interaction model and Plasticity's clean
workflow, with an original look. Nothing is copied (layouts, icons, themes,
keymaps are our own).

## 1. Layout

```
┌ Menu: File Edit View Create Model Sketch Surface Inspect Tools Window Help ──┐
├──────┬──────────────┬─────────────────────────────────────┬──────────────────┤
│ Tool │ Browser      │                                     │ Properties       │
│ bar  │ [Project]    │            3D viewport              │  Selection       │
│      │ [History]    │   grid · axes · orientation cube    │  Active feature  │
│ Sel. │              │   gizmo · preview · hover/select    │  / tool options  │
│ Xfrm │ Solids       │                                     │  Parameters      │
│ Sketch│ Surfaces    │                                     │  (Advanced ▸)    │
│ Solid│ Curves …     │                                     │                  │
│ Surf │              │                                     │                  │
│ Meas.│              │                                     │                  │
├──────┴──────────────┴─────────────────────────────────────┴──────────────────┤
│ Mode: Face │ Selected: Box1.Top │ hints │ X Y Z │ mm │ Recomputed 6 in 4 ms  │
└──────────────────────────────────────────────────────────────────────────────┘
```

PoC implements: left vertical toolbar (selection modes + Fit), Browser dock
with Project and History tabs, Properties dock (Selection, Feature, Parameters,
Mesh reference), status bar with mode, selection, hints, cursor coordinates,
units and recompute status, menus File/View/Help, saved dock layout
(`QSettings`). See `docs/images/`.

Menus only contain implemented commands (brief: no decorative buttons). The
PoC therefore has no Create/Model/Sketch menus yet.

## 2. Interaction model

| Action | Default | Notes |
|---|---|---|
| Orbit | Middle mouse drag (also Alt+Left for laptops) | **[PoC]** |
| Pan | Shift+Middle or Right drag | **[PoC]** |
| Zoom | Wheel at cursor, Ctrl+Middle drag | **[PoC]** |
| Fit all | Home | **[PoC]** |
| Standard views | numpad-style 7 top, 8 front, 9 right, 0 iso (Phase 1: full numpad + view cube) | **[PoC partly]** |
| Selection modes | 1 Object, 2 Vertex, 3 Edge, 4 Face | **[PoC]** |
| Select / extend | Click / Shift+Click | **[PoC]** |
| Transform | G move, R rotate, S scale; then X/Y/Z to constrain; type a value (`10`, `10 mm`, `0.5 in`, `Width/2`); Enter confirm, Esc cancel | Phase 2 |
| Command palette | F3 (and Ctrl+K) | Phase 1 |

Every tool accepts numeric input, every numeric field accepts expressions with
units (PoC: parameter table and feature fields).

Navigation preset alternative (Phase 1): "CAD style" (right-drag orbit) for
users from SolidWorks/Fusion, selectable on first launch.

## 3. Feedback

- Hover highlight (light blue) and selection highlight (amber) consistent across
  modes **[PoC]**.
- Selection is shown in persistent terms (`Box1.Top`,
  `Edge(Box1.Front|Box1.Left)`) **[PoC]**; Phase 1 adds friendly labels
  ("Top face of Box1").
- Feature status dots in History: green ok, red error, grey suppressed/rolled
  back; tooltips show error text **[PoC]**.
- Recompute time and failure count in the status bar **[PoC]**.
- Long operations: progress in status bar + cancel button (Phase 2).
- Errors are specific: "Chamfer failed (size too large for the adjacent
  faces?)", "Reference 'Edge(…)' could not be resolved" **[PoC]**.

## 4. Context-sensitive properties

| Selection | Panel content |
|---|---|
| Face/edge/vertex of a body | Persistent reference + owning feature's parameters **[PoC]** |
| Feature in History | Parameters (expression + evaluated value), edge refs, suppress, status, B-Rep stats **[PoC]** |
| NURBS surface | Surface parameters **[PoC]**; Phase 7: degree, knots, control point table, weights |
| Mesh reference | Analysis report, "not CAD geometry" label **[PoC]**; Phase 6: reconstruction actions |
| Sketch | Constraints, DOF status (Phase 3) |
| Active tool | Tool options + numeric input (Phase 2) |

Progressive disclosure: essentials first, "Advanced" sections collapsed.

## 5. Workspaces

Model (default), Sketch (entered when editing a sketch), Reverse Engineering
(enabled in Phase 6), Assembly (Phase 8). A workspace changes the toolbar
emphasis, not access: every command remains reachable via menus and F3.
A workspace is only added when its functionality exists.

## 6. Visual design

- Dark charcoal palette: window `#2B2D30`, base `#212225`, text `#DCDEE2`,
  dim text `#8A8E94`, accent/selection amber `#FFAA28`, hover blue `#6EB4FF`,
  error `#FF7A6A` **[PoC Theme.cpp]**.
- Viewport: subtle vertical gradient `#34363A → #1E1F22`, light neutral grid,
  solids light grey satin, surfaces muted green, reference meshes translucent
  violet, NURBS cage amber **[PoC]**.
- Typography: Inter (OFL) at 9–10 pt UI, monospace for references.
- Icons (Phase 1): original 20 px line-icon set on a 2 px grid, four states
  (normal, hover, active, disabled), SVG, high-DPI ready. No emoji.
- High-DPI: Qt per-monitor DPI awareness; viewport works in physical pixels
  **[PoC]**.

## 7. Onboarding (Phase 1)

First-launch welcome (choose navigation style, units, theme), a 60-second
interactive tour (select → edit dimension → see recompute), example project,
contextual hint line in the status bar **[PoC: static hints]**, "Don't show
again" everywhere, and a Help → Shortcuts sheet generated from the command
registry.

## 8. Known UX gaps in the PoC

- Text-only toolbar (no icon set yet).
- Feature panel uses free-text expressions without inline validation colours.
- No undo/redo, no gizmo, no command palette (Phase 1–2).
- Face highlight on the NURBS sheet shows the boundary but the fill is subtle.
