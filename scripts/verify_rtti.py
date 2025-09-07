import argparse
import json
import sys
from pathlib import Path


def load_json(path: Path):
    with path.open('r', encoding='utf-8') as f:
        return json.load(f)


def verify(expected, actual):
    missing = []
    mismatched = []
    for cls, methods in expected.items():
        actual_methods = actual.get(cls, {})
        for method, sig in methods.items():
            actual_sig = actual_methods.get(method)
            if actual_sig is None:
                missing.append(f"{cls}::{method}")
            elif actual_sig != sig:
                mismatched.append(f"{cls}::{method} expected {sig} got {actual_sig}")
    return missing, mismatched


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify RTTI method signatures against expected values")
    parser.add_argument("--rtti", required=True, help="Path to RTTI dump JSON")
    parser.add_argument("--version", required=True, help="Game version to check against")
    args = parser.parse_args()

    base = Path(__file__).parent / "rtti" / f"{args.version}.json"
    if not base.exists():
        print(f"Missing expected signatures for version {args.version}: {base}", file=sys.stderr)
        return 1

    expected = load_json(base)
    actual = load_json(Path(args.rtti))

    missing, mismatched = verify(expected, actual)
    if missing or mismatched:
        if missing:
            print("Missing methods:")
            for m in missing:
                print(f"  - {m}")
        if mismatched:
            print("Signature mismatch:")
            for m in mismatched:
                print(f"  - {m}")
        return 1

    print("RTTI signatures match.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
