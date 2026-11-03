"""Use the original generated transition setters in the Options regression."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
functions = []
for address in ("0001A305", "0001A324", "0001AFBA"):
    start = source.index(f"void sub_{address}(void)")
    end = source.index("\n/**", start)
    functions.append(source[start:end])
Path(sys.argv[2]).write_text("\n".join(functions))
