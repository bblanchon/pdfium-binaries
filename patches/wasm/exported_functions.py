import re
import sys
from pathlib import Path

function_pattern = re.compile(r'^FPDF_EXPORT\s+.*?(\w+)\s*\(', re.MULTILINE|re.DOTALL)

def enumerate_functions(folder: Path):
    for file in folder.glob('*.h'):
        content = file.read_text()
        for match in function_pattern.finditer(content):
            yield "_" + match.group(1)

if __name__ == "__main__":
    folder = Path(sys.argv[1])
    print(",".join(enumerate_functions(folder)))
