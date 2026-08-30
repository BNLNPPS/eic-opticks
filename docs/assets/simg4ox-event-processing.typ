// Regenerate from the repository root with:
//   typst compile --format svg docs/assets/simg4ox-event-processing.typ \
//     docs/assets/simg4ox-event-processing.svg

#let ink = rgb("#17324d")
#let muted = rgb("#5d6975")
#let panel-fill = rgb("#f8fafc")
#let panel-stroke = rgb("#cbd5df")
#let cpu-fill = rgb("#dceeff")
#let cpu-stroke = rgb("#3978a8")
#let gpu-fill = rgb("#eee3ff")
#let gpu-stroke = rgb("#7651a8")
#let wait-fill = rgb("#f0f2f4")
#let wait-stroke = rgb("#8a929b")
#let output-fill = rgb("#e3f5e8")
#let output-stroke = rgb("#3d8b59")

#set page(width: 1050pt, height: auto, margin: 22pt, fill: white)
#set text(font: "DejaVu Sans", size: 10pt, fill: ink)
#set par(leading: 0.7em)

#let timeline-columns = (
  90pt,
  1fr, 1fr, 1fr, 1fr, 1fr, 1fr,
  1fr, 1fr, 1fr, 1fr, 1fr, 1fr,
)

#let lane-label(body) = align(
  right + horizon,
  text(size: 8.5pt, weight: "bold", fill: ink, body),
)

#let segment(fill-color, border-color, body) = box(
  width: 100%,
  height: 29pt,
  fill: fill-color,
  stroke: 0.8pt + border-color,
  radius: 4pt,
  inset: (x: 3pt, y: 4pt),
  align(center + horizon, text(size: 8pt, weight: "bold", fill: ink, body)),
)

#let panel(title, subtitle, body) = block(
  width: 100%,
  fill: panel-fill,
  stroke: 0.8pt + panel-stroke,
  radius: 7pt,
  inset: 12pt,
)[
  #text(size: 12pt, weight: "bold")[#title]
  #h(8pt)
  #text(size: 8.5pt, fill: muted)[#subtitle]
  #v(8pt)
  #body
]

#align(center)[
  #text(size: 17pt, weight: "bold")[Current `simg4ox` event processing]
  #v(2pt)
  #text(size: 9pt, fill: muted)[E0–E2 denote complete Geant4 events, each potentially containing many G4 tracks. The GPU event context is process-wide in both modes.]
]

#v(12pt)

#panel(
  [Serial (`--threads 1`)],
  [Each Geant4 event and its GPU photon transport finish before the next event starts.],
  grid(
    columns: timeline-columns,
    column-gutter: 2pt,
    row-gutter: 5pt,
    align: horizon,

    [],
    grid.cell(colspan: 12)[#align(right)[#text(size: 7.5pt, fill: muted)[time →]]],

    [#lane-label[Geant4 CPU]],
    grid.cell(colspan: 2)[#segment(cpu-fill, cpu-stroke)[process event E0]],
    grid.cell(colspan: 2)[],
    grid.cell(colspan: 2)[#segment(cpu-fill, cpu-stroke)[process event E1]],
    grid.cell(colspan: 2)[],
    grid.cell(colspan: 2)[#segment(cpu-fill, cpu-stroke)[process event E2]],
    grid.cell(colspan: 2)[],

    [#lane-label[Shared GPU]],
    grid.cell(colspan: 2)[],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E0 photons]],
    grid.cell(colspan: 2)[],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E1 photons]],
    grid.cell(colspan: 2)[],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E2 photons]],
  ),
)

#v(10pt)

#panel(
  [Multithreaded (`--threads 3`)],
  [Geant4 events are processed concurrently; their photons enter GPU transport strictly in event-ID order.],
  grid(
    columns: timeline-columns,
    column-gutter: 2pt,
    row-gutter: 5pt,
    align: horizon,

    [],
    grid.cell(colspan: 12)[#align(right)[#text(size: 7.5pt, fill: muted)[time →]]],

    [#lane-label[Worker 0]],
    grid.cell(colspan: 3)[#segment(cpu-fill, cpu-stroke)[process event E0]],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E0 photons]],
    grid.cell(colspan: 7)[],

    [#lane-label[Worker 1]],
    grid.cell(colspan: 2)[#segment(cpu-fill, cpu-stroke)[process event E1]],
    grid.cell(colspan: 3)[#segment(wait-fill, wait-stroke)[wait for E0]],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E1 photons]],
    grid.cell(colspan: 5)[],

    [#lane-label[Worker 2]],
    grid.cell(colspan: 4)[#segment(cpu-fill, cpu-stroke)[process event E2]],
    grid.cell(colspan: 3)[#segment(wait-fill, wait-stroke)[wait for E0–E1]],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E2 photons]],
    grid.cell(colspan: 3)[],

    [#lane-label[Shared GPU]],
    grid.cell(colspan: 3)[],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E0 photons]],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E1 photons]],
    grid.cell(colspan: 2)[#segment(gpu-fill, gpu-stroke)[transport E2 photons]],
    grid.cell(colspan: 3)[],
  ),
)

#v(10pt)

#align(center)[
  #box(width: 11pt, height: 11pt, fill: cpu-fill, stroke: 0.8pt + cpu-stroke, radius: 2pt)
  #h(4pt) Geant4 event processing
  #h(18pt)
  #box(width: 11pt, height: 11pt, fill: wait-fill, stroke: 0.8pt + wait-stroke, radius: 2pt)
  #h(4pt) Waiting for event-ID turn
  #h(18pt)
  #box(width: 11pt, height: 11pt, fill: gpu-fill, stroke: 0.8pt + gpu-stroke, radius: 2pt)
  #h(4pt) Serialized GPU photon transport
  #h(18pt)
  #box(width: 11pt, height: 11pt, fill: output-fill, stroke: 0.8pt + output-stroke, radius: 2pt)
  #h(4pt) Run-end merge and save
]

#v(8pt)

#align(center)[
  #box(
    fill: output-fill,
    stroke: 0.8pt + output-stroke,
    radius: 4pt,
    inset: (x: 10pt, y: 5pt),
  )[
    After all events: merge hits by event ID → `s_hits.npy` + `g_hits.npy`
  ]
]