# TODO

The 2027 plan: scope, milestones and how a milestone runs. **Task status lives in Obsidian**, in the
*Caldera Renderer — Development Roadmap* base: one note per `CLR-N` ticket with `Status`,
`Milestone`, `Priority` and `Est. Hours`. Long-term phases are in `ROADMAP.md`, which is not used
for planning in 2027.

## Caldera 2027 — scope

**Final demo (Dec 2027):** Sponza (Khronos glTF Sample Models) with hundreds of point lights, a
free camera, an ImGui toggle per technique, a per-pass ms table from GPU timestamps / Tracy, and a
README with a frame breakdown.

The table names each milestone's headline items; the full list is the `Milestone` property in Obsidian.

| Milestone | Target | Items |
|---|---|---|
| **A** | Nov 2026 – Feb 7, 2027 | CLR-49 Camera, CLR-48 Sync audit, CLR-45 Tracy, CLR-50 Multi-mesh scene loading, CLR-14 PBR, CLR-51 HDR + tonemapping |
| **B** | Feb 8 – Mar 14 | CLR-11 Topological sort, CLR-13 Transient resources, CLR-16 Shadow mapping |
| **C** | Mar 15 – May 30 | C1: CLR-17 GPU culling + stream compaction. C2: CLR-52 Deferred, CLR-53 Clustered lighting + ms table |
| **D** | Jun – Aug | One of IBL (CLR-23–25 + CLR-15), CSM or SSAO — chosen at the end of May |
| **E** | Sep – Oct | CLR-54 TAA |
| **F** | Nov – Dec, stretch | CLR-55 LBVH + compute shadows, hardware ray query (CLR-28) as the reference |

**Not in 2027:** CLR-26 Threading, CLR-46 Shader reflection, CLR-18–21 (full bindless, async
compute, hot reload, mesh shaders), RT reflections, virtual shadow maps, scene graph, a full material
system, side-topic mini-programs. Tickets with `Milestone: As needed` are done only when a scoped
item needs them.

**How a milestone runs:**

1. Topic → reading (books in `../Graphics`, articles, docs) → quiz.
2. Design page → AI review.
3. Work-style tickets from the AI (task, acceptance criteria, constraints). Core code is written by
   hand and reviewed by the AI; scaffolding may be generated. Same checklist as at work: a first
   patchset I would merge, numbers in the commit message.
4. Last ticket is a bug: the AI plants one in my code, I find it with RenderDoc and validation layers.

**Done means:** builds with one command, screenshots or GIF, README with decisions, trade-offs and
numbers, one GPU-timestamp number, and a line on what was written by hand vs generated.

**Planning:** once per cycle, 30 minutes on its first evening; switch the *Current milestone* view in
Obsidian to the new letter. Every other evening starts at the first open ticket in that view.
