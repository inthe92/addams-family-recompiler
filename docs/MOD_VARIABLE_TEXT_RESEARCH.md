# Variable-length text — bounded research, no pointer patcher

2026-10-14, local `feature/modding-sdk-v2-phase4`; historical HEAD
`95c194afac4574be9bf8b495341517389a73988b` plus preserved SDK1–3 dirty work.
No production, generated private C, ABI3 or ROM change in Phase 4.

## Existing contract and verified scope

The current approved resources are `inventory.heading` (nine encoded cells)
and `hud.life` (four). Logical text can be shorter, but `encode` in
[resources.py](../tools/mods/resources.py):64 pads to the **same fixed encoded
length**. `validate_profile` (line 17) and `compile_translation` (line 82)
exclude control bytes and require exact bounded replacements. This is already
functional, not unbounded variable-length text. The Phase 4 owner-image check
reconfirmed both approved fields' original SHA/CRC identities and their actual
background font consumers; no original byte dumps are distributed.

The existing private format investigation from SDK Phase 1/2 documented
pointer-driven PPU data segments, a 1,024-byte nametable payload plus terminator,
two dialogue PPU segments containing 14 glyph codes and an eight-bit cursor.
Seven instruction/operand checks were part of that earlier investigation.
Those are **historical evidence**, not a fresh complete-stream replay in this
phase. See [Phase 1](MODDING_SDK_V2_PHASE_1.md),
[Phase 2](MODDING_SDK_V2_PHASE_2.md) and [Phase 3](MODDING_SDK_V2_PHASE3.md).
No exhaustive compression classification or free-space map has been established.

## Why increasing a payload is not simply adding characters

Streams can contain destinations, formatting controls, lengths and terminators
next to glyph data. Extra bytes can replace an adjacent field/control/border,
change the PPU cursor or wrap an eight-bit reader. A moved terminator can change
the number of CPU instructions, data reads and `$2007` writes. Repointing data
must also preserve PRG bank context and every reader. Even a visual-only intent
does not imply equal CPU paths or frame timing.

Generated instructions, immediate operands and some targets are compiled into
C constants. Existing data-bus overlays do not intercept all these accesses;
arbitrary pointer edits cannot be inferred to redirect compiled execution.
RMW reads/old/new writes have ordered side effects. Do not turn the resource
layer into an instruction or hardware-register patch service. See the historical
[bus access inventory](BUS_TIMING_REPORT.md) and current emitter
[emit_blocks.py](../tools/recompiler/emit_blocks.py), functions `emit_address`,
`emit_semantics` and `emit_image`.

## Alternatives and evidence status

| Option | Classification | Conditions / missing evidence |
|---|---|---|
| Shorter logical text padded inside an approved fixed field | VIABLE, implemented | Existing glyph mapping, same length and original field fingerprints |
| Repack a reviewed fixed-size PPU data block | EXPERIMENTAL | Full controls/reader model, neighboring field layout and unchanged stream bounds; CPU/PPU trace and raster proof |
| Immutable virtual data buffers with a symbolic resource reader | EXPERIMENTAL | Explicit bank/cursor/lifetime mapping, executable exclusions and policy for changed read count/timing; not a generic address interceptor |
| Declarative per-reader relocation plan | EXPERIMENTAL | All cross-references must be known, including baked constants; incompatible readers rejected; no public executable metadata |
| Generic PRG writes, jump/vector/instruction patching or private cache edits | RECHAZADO | Outside resource contract and preservation constraints |
| Per-language C++ patches / per-mod host rebuilds | RECHAZADO for the SDK design | Does not meet reusable package objective |
| Extend every dialogue, reuse supposedly free PRG, compressed text substitution | PENDIENTE DE PRUEBAS | Compression, pointer graph, reader widths, bank transitions and region ownership incomplete |
| Full Spanish typography with new CHR glyphs | PENDIENTE DE PRUEBAS | First establish a safe destination; see [CHR audit](MOD_CHR_RESOURCE_AUDIT.md) |

## Future resource/profile model

A versioned trusted profile could describe symbolic streams, fixed allocation,
allowed glyph/control grammar, cursor width and exact immutable original
fingerprints. Untrusted language JSON supplies only text for approved IDs;
neither pointers nor CPU opcodes. Compiler/validation tooling must distinguish
rendered Unicode width from byte length. Reject unknown glyphs and unsupported
layout instead of stripping accents, truncating or wrapping silently.

Larger virtual streams need bounded owned buffers, atomic text/glyph/codec
publication, exclusive resource owners and a screen reload/lifetime model. No
partial publication or dangling reader on disable. An unchanged ABI3 structure
size is not enough to establish compatibility if its execution contract changes;
any future such change needs explicit version/cache review.

## Minimal next experiment and acceptance

1. Finish the safe CHR destination proof for one glyph and one fixed field first.
2. Choose one non-executable, fixed-capacity stream, using the owner's verified
   image privately. Identify every pointer reader, CPU bank, control/terminator,
   bounded output cell and cross-reference without exporting game data.
3. Demonstrate an equal-sized alternate layout with authored text only; compare
   original/active/disabled CPU registers/cycles, RAM, PCM, IRQ/NMI/DMA and the
   affected PPU region. Record intentional image differences separately.
4. Only then decide whether more glyphs fit within existing capacity or require
   changed execution. Stop before implementing redirection if the model depends
   on an unverified reader or pointer embedded in executable instructions.

No pointer rewriting, compression service, virtual stream backend or longer
in-game text was implemented or demonstrated in this phase. The research is a
bounded design and explicit evidence gap, not a functioning translation feature.
