"""Use original generated menu transitions and hit-box helpers in regressions."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
functions = []
for address in ("0001A305", "0001A324", "0001AFBA", "00025F30", "00025F40", "0001C953", "0001B237"):
    start = source.index(f"void sub_{address}(void)")
    end = source.index("\n/**", start)
    functions.append(source[start:end])
Path(sys.argv[2]).write_text("\n".join(functions))
