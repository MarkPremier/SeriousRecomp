"""Extract the user's generated shadow-mask routine for the native regression.

No disc code is checked in. Reconfigure after regenerating the title.
"""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
start = source.index("void sub_00294650(void)")
end = source.index("\n/**", start)
Path(sys.argv[2]).write_text(source[start:end])
