"""Extract the user's generated CRT remainder loop; no disc code is committed."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
start = source.index("void sub_002B6724(void)")
end = source.index("\n/**", start)
Path(sys.argv[2]).write_text(source[start:end])
