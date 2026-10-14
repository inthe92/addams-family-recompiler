# Mod package and Lua restrictions

2026-10-13. These are tested defenses, **not an OS sandbox or a guarantee that hostile code cannot exploit a Lua/interpreter/host vulnerability**. Use reviewed mods. The legacy developer `--mod` option has different trust assumptions and must not be described as API v1 isolation. `--no-mods` bypasses both paths.

## ZIP/storage

- Maximum archive 8 MiB, 128 entries, expanded total 16 MiB, script 128 KiB and manifest 8 KiB. Expansion ratio limited after a 1 MiB small-file allowance; extraction also checks actual streamed sizes/CRC. At most 32 installed mods.
- Only stored/deflated ZIPs; no encryption. Reject absolute/traversal paths, empty/dot components, backslashes, drive/ADS colons, reserved Windows names, trailing dots/spaces, long/non-ASCII paths, case-insensitive duplicates and file/directory collisions.
- Validate `ZipInfo.orig_filename` before trusting `filename`: CPython normalizes Windows separators and truncates NUL in the latter. Refuse differing normalized/alternate names. The synthetic malformed-name writer preserves raw names on every platform; a normalization control and embedded-NUL path retain their rejection expectations. This was corrected after the first native Windows CI exposed the issue. See [CPython v3.14.7 ZipInfo and filename sanitization](https://github.com/python/cpython/blob/v3.14.7/Lib/zipfile/__init__.py#L373-L422).
- Reject symlinks/special entries. Extraction creates fresh regular files with exclusive creation; it does not apply executable permissions or link metadata from ZIP headers. Native extensions and unexpected files are refused; common executable/native/bytecode signatures are rejected during extraction.
- Entry must be UTF-8 text without NUL. Syntax errors are contained when enabling, not misreported as successful script execution.
- Validate before staging; stage in a unique private directory on the same filesystem and rename only after successful extraction. Existing IDs are never overwritten; uninstall before replacing. Failed extraction removes only its own staging directory.
- Root/package/state symlinks are refused. State writes use temporary files, flush/fsync and replacement. Mutations use an exclusive directory lock; a stale crash lock needs manual review. No automatic deletion of unknown user files.
- Uninstall persists deactivation first, then renames the owned package out of the installed tree before removal. An interrupted removal may leave a private `.removed-*` directory; it is not auto-loaded or silently cleaned.

These controls assume the user owns a trustworthy local storage directory. They do not establish race-proof protection against another process running as that same user, mount changes or external filesystem tampering. Manually corrupted installed manifests currently stop the package listing with an error; safe mode remains available and repair/removal requires local review.

## Lua

Each mod owns a distinct Lua state. Its allocator caps Lua allocations at **2 MiB** (up to 64 MiB for 32 states); C++ snapshots/log containers are bounded separately. A count hook every 1,000 VM instructions enforces a **100,000 instruction** callback/chunk budget and checks a **20 ms monotonic deadline**. The deadline is a sampled guard, not hard real-time preemption of C library calls.

No io, os, package, debug or coroutine library is opened. Base load/dofile/loadfile, pcall/xpcall, garbage-collector controls and metatable access are removed. Without script-visible protected calls the mod cannot catch a budget error and keep looping. String patterns, bytecode dumping/packing, replication/formatting, table.sort and random APIs are omitted. Lua source loading is text-only. The C++ host remains responsible for installation/logging; no script can call that transport.

Bounded log ring: 128 messages per mod, 300 characters each. The host drains new messages to private `mods.log`, capped by truncating this manager-owned log after ~1 MiB. Lua/state errors disable only the failed mod in memory, record an error and attempt atomic persistent deactivation; other mods and native execution continue. Failures to persist are reported. Session-marker creation must succeed before loading scripts. An unclean session disables previously enabled mods on the next managed start rather than silently retrying them. Force-termination cannot promise `on_unload`; recovery is conservative.

## Tested / remaining

Synthetic tests exercise traversal, reserved/duplicate paths, links, collisions, executables disguised as assets, limits, malformed/CRC-damaged ZIP rollback, state persistence, syntax errors, infinite loops, memory exhaustion, blocked library/environment access, isolation and failure recovery. Managed callbacks cannot mutate the injected observation or generated native program state in the tested rates.

The callback deadline does not include filesystem log flushing or package operations. Heavy scripts/logging can add host latency; a bounded VM budget is not a guarantee of maintaining wall-clock PAL pacing under arbitrary mod load.

Still pending: adversarial fuzzing, independent security review, simultaneous application instances using one store, live Windows/Linux filesystem behavior, physical Steam Deck controls and broader CPU/C-library time profiles. Do not infer complete safety or constant frame time from the unit tests. No scripts, logs, game content or user caches are uploaded in this task.

## API v2 first vertical delivery

Explicit observe.player/overlay.text permissions and engine/profile requirements
are checked before install/enable and again at startup. No host writes exposed.
HUD arguments require callback context, exact types, supported text and complete
fit in a 256×40 strip; at most 8 commands × 40 characters per mod, 32 mods. Host
lists/bitmap are bounded separately from Lua memory. Callback failure removes
partial and stale lists. Rendering uses captured data and never executes Lua.
No new executable/assets decoder, filesystem API or online distribution channel.
This extends the same in-process restrictions, not the OS trust boundary.

## Completion01 recovery and declarative state

Metadata/configuration/profile operations are local, bounded and script-free.
Eight options per mod, sixteen named profiles, state <=65,000 serialized bytes.
Types/bounds/enum membership/unknown keys are validated before atomic replace;
profile names are printable bounded labels, never filesystem paths. State remains
in the existing state.json; old enabled/errors state is read without destructive
migration. Original is built-in and cannot be renamed; --no-mods still bypasses
managed storage/helpers entirely.

An individually damaged installed manifest/entry now becomes a disabled error
record; other intact mods remain manageable and the bad package can be uninstalled.
Unsafe storage roots/identities and globally corrupt state still fail closed for
manual review. Linked files are never loaded through this recovery path. Existing
ZIP/path/sandbox limits remain unchanged. Profile validation and JSON persistence
are transactional, but Lua activation follows commit: a script failure disables
that owner/dependents and records errors, rather than claiming all VMs activated
atomically. A previous altered jump trajectory is not reversed by a profile change.

## Completion02 profile exchange and diagnostic packaging

Portable profile JSON is limited to64KiB/32mods and strictly checked for duplicate
keys, unknown fields, exact compatibility, setting types/ranges, dependencies and
conflicts. Symlink/nonregular inputs are rejected. Inspection retains a SHA256
which is rechecked at import, preventing an altered file from being accepted as
the reviewed document. Names are case-insensitively unique; Original is immutable.
Import never enables mods or executes Lua. Deleted profiles do not delete mods.

Exports use a temporary file, fsync and an atomic no-overwrite hard-link publish;
existing files are refused. A filesystem without hard links can reject export:
export locally and copy the finished JSON. This does not establish crash durability
on every filesystem, eliminate all OS-level races or provide an OS sandbox.
Lua cannot access the selected host path. The existing JSON store remains the
only manager persistence. Corrupt user state is reported, not silently erased.

A separate diagnostic packaging gate permits only complete mandatory CTest/JUnit
results with no omitted/skipped tests or unexpected failures. A02 remains ordinary
FAIL and the strict suite gate still fails. Diagnostic eligibility is not suite
PASS or release approval. Public source exports use the reviewed explicit file
list and SHA/provenance records; production publication remains unauthorized.
