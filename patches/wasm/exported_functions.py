import subprocess
import sys

def enumerate_symbols(files: list[str]):
    result = subprocess.run(["llvm-nm", "--format=just-symbols"] + files, capture_output=True, text=True)
    for symbol in result.stdout.splitlines():
        if symbol.startswith(("FPDF", "FSDK", "FORM", "IFSDK")):
            yield f"_{symbol}"


if __name__ == "__main__":
    print(",".join(enumerate_symbols(sys.argv[1:])))
