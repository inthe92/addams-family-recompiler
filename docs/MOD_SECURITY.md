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
