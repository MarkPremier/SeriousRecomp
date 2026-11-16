"""Extract local generated helpers for cheat integration tests, without disc code in Git."""
from pathlib import Path
import re, sys

gen = Path(sys.argv[1])
functions = []
for chunk, address in (("recomp_0004.c", "0004C580"), ("recomp_0015.c", "000CA160")):
    source = (gen / chunk).read_text()
    match = re.search(rf"void sub_{address}(?:_gen)?\(void\)", source)
    if not match:
        raise SystemExit(f"Missing helper {address}; regenerate with the manual overrides")
    end = source.index("\n/**", match.start())
    functions.append(source[match.start():end])
Path(sys.argv[2]).write_text("\n".join(functions))
