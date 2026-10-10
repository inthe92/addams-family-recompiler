# Publishing

[Español](PUBLISHING.es.md)

This source export uses an explicit allowlist rather than copying the private research project. Dependency archives and bundled runtime tools have pinned SHA-256 checksums.

Allowed: original engine, generic translator, address/hash metadata, original patches, verified SDL/Lua archives, synthetic tests, documentation, original icon and CI workflow. Excluded: ROM, recovered ASM, CFG containing bytes/operands, translated banks, import caches/modules, extracted maps/collision/CHR/audio, captures, snapshots and the old embedded-content alpha.

Before a commit, from the public folder:

```sh
python3 -B tools/public/audit_public.py .
python3 -B tests/public_import_checks.py
```

Do not run publication commands from the private research folder. Never attach a ROM, private cache or previous alpha to an issue or release. Release packaging accepts only the content-free build and verified runtime tools. Toolchain/cache/build folders are ignored by Git.

The `Build content-free frontends` workflow runs on pushes, pull requests or manual dispatch. It uploads platform artifacts but does not publish releases automatically. Runners have no game ROM. Synthetic CI checks do not establish full-game correctness.

The audit rejects common private paths/assets and unexpected profile fields. Its optional local `--reference-rom` checks raw byte markers from the input. It is a technical check with limited scope, not legal certification. MIT covers original contributions, never commercial game content.
