# Verification status

[Español](VERIFICATION.es.md)

Prepared October 10, 2026. The exact local delivery report is `verification-public.json`, alongside the public source ZIP and platform packages.

The engine builds without a ROM. Source contract tests use synthetic data and verify profile fields, supported official opcodes and rejection of incompatible images before generating content. A separate synthetic fixture checks bundled native-module compilation/loading on the executing host.

Local frontends have been built for macOS ARM64, Windows x64 and Linux x64. Windows/Linux are cross-built on macOS; compilation and package inspection do not establish execution on those systems. The macOS import/boot probe uses a privately supplied ROM, never included in source or release assets. The report states which checks actually ran.

Pending: hardware tests on Windows and Steam Deck, complete gameplay/routes/audio correctness, all death recovery cases, and widescreen enemy activation and fixed-width/vertical room behavior. Mac Intel is configured in CI but has not been locally tested. This release is an alpha, not a completed port.
