# RTTI Signatures

Versioned method signatures from game RTTI dumps.

Each `<version>.json` file stores expected signatures for that game build.
Run `scripts/verify_rtti.py --rtti <dump.json> --version <version>` during CI to
ensure scripted calls remain valid.
