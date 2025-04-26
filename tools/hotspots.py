import math
import re

hotspots: dict[str, int] = {}

gradient = [
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
]

with open("hotspots.txt") as fp:
    for line in fp:
        addr = line.strip()

        if addr in hotspots:
            hotspots[addr] += 1
        else:
            hotspots[addr] = 1

total: int = sum(hotspots.values())
worst: int = max(*hotspots.values())

disassembly: list[str] = []


with open("disassembly.txt") as fp:
    for line in fp:
        disassembly.append(line.rstrip())

stats: dict[str, int] = {}
symbol: str = "__root__"
stats[symbol] = 0
running = 0
prev_was_code = False

for lineno, line in enumerate(disassembly):
    if not line:
        continue

    m = re.match(r"^ *[0-9a-fA-F]{4,} <([^>]+)>:", line)
    if m:
        symbol = m.group(1)
        stats.setdefault(symbol, 0)
        running = 0
        if prev_was_code:
            print("\x1b[38;5;240m───────────────┴" + "─" * 70 + "\x1b[0m")
            prev_was_code = False

        print(f"\x1b[1;31m.{symbol}:\x1b[0m")
        continue

    if not line:
        print(line)
        continue

    if not re.match(r"^ *[0-9a-fA-F]{4,}:", line):
        if prev_was_code:
            print("\x1b[38;5;240m───────────────┴" + "─" * 70 + "\x1b[0m")
            prev_was_code = False
        print(line)
        continue

    m = re.match(r"^ *([0-9a-fA-F]{4,}):", line)
    assert m

    addr = m.group(1)
    line += " "

    if addr in hotspots:
        count = hotspots[addr]
        stats[symbol] += count
        running += count
        badness = math.log(count) / math.log(worst)
    else:
        count = 0
        badness = 0

    perc = 100 * count / total
    cumm = 100 * running / total

    color = gradient[int(badness * (len(gradient) - 1))]
    escape = f"\x1b[38;5;{color:03}m"

    if not prev_was_code:
        prev_was_code = True
        print("\x1b[38;5;240m───────────────┬" + "─" * 70 + "\x1b[0m")

    print(f" {escape}{cumm:5.2f}% {perc:5.2f}% │ {line}\x1b[0m")

print("")
print("")
print("Summary:")

for name, count in sorted(stats.items(), key=lambda s: -s[1]):
    if count <= 1:
        break

    print(f"  {100 * count / total:5.2f}%  {name}")
