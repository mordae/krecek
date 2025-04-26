#
# 1. Obtain list PC values and store them in hotspots.txt.
#
# 2. Obtain PC-to-line mapping:
#
#    sort -u <hotspots.txt \
#        | arm-none-eabi-addr2line -e build/krecek/voxels/voxels.elf -af \
#        >lines.txt
#
# 3. Run this tool:
#
#    python tools/profile.py | less -RS
#

import math
import os
import os.path
import re

GRADIENT = (
    27,
    33,
    39,
    45,
    51,
    50,
    49,
    48,
    47,
    46,
    82,
    118,
    154,
    190,
    226,
    220,
    214,
    208,
    202,
    196,
)

hotspots: dict[int, int] = {}
total: int = 0
worst: int = 0

with open("hotspots.txt") as fp:
    for line in fp:
        addr = int(line.strip(), 16)
        total += 1

        if addr in hotspots:
            hotspots[addr] += 1
        else:
            hotspots[addr] = 1


line_map: dict[int, tuple[str, str, int]] = {}

with open("lines.txt") as fp:
    lines = [line.rstrip() for line in fp.readlines()]

    for i in range(0, len(lines), 3):
        addr = int(lines[i + 0].strip(), 16)
        function = lines[i + 1].strip()
        location = lines[i + 2].strip()

        m = re.match(r"^([^:]*):([0-9]+)", location)
        if not m:
            continue

        filename = m.group(1)
        lineno = int(m.group(2))
        line_map[addr] = (filename, function, lineno)

current_directory = os.path.realpath(os.getcwd())

file_heatmap: dict[str, dict[int, int]] = {}
function_heatmap: dict[str, int] = {}

for addr, (filename, function, lineno) in line_map.items():
    if os.path.exists(filename):
        relpath = os.path.relpath(filename, current_directory)
        file_heatmap.setdefault(relpath, {})
        file_heatmap[relpath].setdefault(lineno, 0)
        file_heatmap[relpath][lineno] += hotspots.get(addr, 0)
        function_heatmap.setdefault(function, 0)
        function_heatmap[function] += hotspots.get(addr, 0)

for heatmap in file_heatmap.values():
    worst = max(worst, *heatmap.values())


def generate_report(relpath: str, heatmap: dict[int, int], total: int, worst: int):
    summary = sum(heatmap.values())
    if summary < total / 100:
        return

    percentage = 100 * summary / total
    header = f"--- {relpath} ({percentage:.3f}%) "
    print(header.ljust(80, "-"))

    log_worst = math.log(worst) if worst else 1

    with open(relpath) as fp:
        for lineno, line in enumerate(fp):
            line = line.rstrip()
            hits = heatmap.get(lineno + 1, 0)
            percentage = 100 * hits / total

            badness = math.log(hits) / log_worst if hits else 0
            color = GRADIENT[round(badness * (len(GRADIENT) - 1))]

            print(
                f"\x1b[38;5;{color:03}m{lineno + 1:4} {percentage:6.3f}%  {line}\x1b[0m"
            )

    print()


sorted_files = list(sorted(file_heatmap.items(), key=lambda f: -sum(f[1].values())))
sorted_functions = list(sorted(function_heatmap.items(), key=lambda f: -f[1]))

print("Files:")

cumper: float = 0

for relpath, heatmap in sorted_files:
    percentage = 100 * sum(heatmap.values()) / total
    cumper += percentage

    print(f"  {cumper:6.3f}%  {percentage:6.3f}% ", relpath)

    if cumper >= 95:
        break

print()

print("Functions:")

cumper: float = 0

for function, hits in sorted_functions:
    percentage = 100 * hits / total
    cumper += percentage

    print(f"  {cumper:6.3f}%  {percentage:6.3f}% ", function)

    if cumper >= 95:
        break

print()


for relpath, heatmap in sorted_files:
    generate_report(relpath, heatmap, total, worst)
