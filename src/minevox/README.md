# Minevox

A voxel survival game for the Krecek handheld. Mine, craft, build, fight
things at night, and light a nether reactor.

This document is a guided tour of the code. Every source file is heavily
commented, but comments explain *lines*; this explains *why the program is
shaped the way it is*. If you want to learn how a small 3D game works,
read this first and then follow the reading order below.

---

## The machine we are writing for

Almost every decision in this codebase comes from these five numbers.

| | |
|---|---|
| Screen | 160 × 120 pixels, 16-bit colour |
| RAM | 264 KiB, all of it, forever |
| Flash | 512 KiB for this game |
| CPU | Cortex-M0+ at 133 MHz, **no floating point unit** |
| Input | one analog stick, six buttons, a touch panel |

Two of those deserve emphasis.

**264 KiB of RAM is the whole budget.** There is no swap, no virtual
memory and no operating system to ask for more. The double-buffered
screen alone takes 77 KiB before we write a line of game code. Every array
in this program was sized against that number, and you will find the
arithmetic written out wherever it mattered.

**No floating point unit** means every `float` operation is a function
call into a software library. A multiply is roughly 25 cycles instead of
one. This is why the renderer computes a ray direction by *adding* a
constant vector per pixel rather than recomputing it, and why lighting is
done in integers from 0 to 32 rather than in 0.0 to 1.0.

---

## Building and running

```bash
# Desktop, which is where you should do almost all your work
cmake -B build/host host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host --target minevox
build/host/minevox/minevox

# The console
cmake -B build src -G Ninja
cmake --build build --target minevox
picotool load -f build/minevox/minevox.uf2
```

A debug build with the address sanitizer catches memory bugs that would
silently corrupt the console:

```bash
cmake -B build/host-dbg host -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host-dbg --target minevox
ASAN_OPTIONS=detect_leaks=0 build/host-dbg/minevox/minevox
```

## Controls

| Console | Desktop | Does |
|---|---|---|
| stick | W A S D | walk forwards and back, turn |
| L / R | Q / E | look down, look up |
| A | L | jump; double tap to fly in creative |
| B | P | mine what you look at, or hit a mob (hold) |
| X | K | use: place a block, eat food, light a reactor |
| Y | O | tap for the next hotbar slot, hold to open the inventory |
| START | Enter | pause menu |
| touch | mouse | every menu can also be tapped |

---

## Reading order

Read them in this order and each one only depends on what came before.

### 1. `mem.h` and `mem.c` — where memory comes from

Start here, because it is short and it explains a habit that runs through
the whole program: we never call `malloc`. Instead there is one static
array, an **arena** that hands out memory by moving a pointer forward, and
**pools** of fixed-size slots that can be freed in any order.

The idea worth stealing is the **free list**: a pool tracks which slots are
free by storing the index of the next free slot *inside the free slot
itself*. Bookkeeping that costs nothing, because free memory is not doing
anything else.

### 2. `world.c` — voxels, noise and the ray march

Three things live here.

**Storage.** One byte per voxel in a flat array. `world_index()` turns
(x, y, z) into a single offset. That is the entire data structure. There
is no chunk, no octree, no compression, because at this size a flat array
beats all of them and is far easier to reason about.

**Terrain from noise.** The comment above `hash_coords` walks through value
noise from scratch: hash the corners of a grid, blend between them, bend
the blend with a smoothstep so the seams disappear, then stack several
layers at doubling frequency. That last trick is called fractional
Brownian motion and it is how most procedural landscapes are made.

**The DDA.** `world_raycast` is the heart of the game. Read its comment
carefully. Given a ray, it visits exactly the voxels the ray passes
through, in order, with a couple of comparisons per step. Understand this
one function and the renderer stops being mysterious.

### 3. `render.c` — turning voxels into pixels

The renderer casts **one ray per screen pixel**, all 19200 of them, using
the same DDA. There are no triangles anywhere in this program.

Things to notice:

- **Why per pixel and not per column.** Wolfenstein-era raycasters used one
  ray per column and drew a vertical strip, which is why you could never
  look up in them. Per-pixel costs 120× more rays and buys a completely
  free camera: any pitch, floors, ceilings, blocks above you, all from the
  same loop.
- **Incremental ray directions.** Walking down a screen column adds a
  constant vector each pixel instead of recomputing from angles. On a chip
  with no FPU that is the difference between playable and not.
- **Face shading.** There is no lighting model. Each of the six directions
  gets a fixed brightness. Your eye reads that as three-dimensional
  immediately, and it costs one table lookup.
- **Skylight without ray tracing.** How dark a block is depends on how
  deeply it is buried, which `world_height` already knows. No shadow rays.
- **The depth buffer.** `render_world` records how far away each pixel is,
  and then sprites test against it. This is a **z-buffer**, the same idea a
  graphics card uses. Ours is one byte per *four* pixels, because a full
  one would cost 19 KiB we do not have.
- **Billboards.** Mobs and dropped items are flat images that turn to face
  the camera, exactly as Doom did. `draw_sprite` projects a world point to
  the screen with three dot products, which works because the camera basis
  is orthonormal.

### 4. `player.c` — physics that feels right

Collision is a box against blocks, tried **one axis at a time**. That single
detail is why you slide along a wall instead of stopping dead, and it is
about ten lines of code.

Also here: fall damage measured from the height a fall started rather than
from speed, automatic stepping up single blocks (because turning and
walking share one stick), and how mining time depends on the tool you hold.

### 5. `items.c`, `mobs.c` — things that are not blocks

Both use pools from `mem.c` and a **singly linked list** of the live ones.
Watch how `items_update` removes entries while walking the list, using a
pointer to a pointer so the head needs no special case. That is a small
piece of C worth learning properly.

Mob behaviour is deliberately dumb: wander, or walk at the player if
hostile and close. No pathfinding at all. It reads as alive, and it costs
nothing.

### 6. `save.c` — a world in four kilobytes

The world is 75 KiB. A save slot is 4 KiB. The trick is that a world is
not really data, it is the **output of a function**: the same seed always
generates the same island. So we save the seed plus a journal of the
blocks the player changed, and rebuild the rest on load.

This is the same idea as an event log in a database, and it is worth
remembering whenever data is expensive to store but cheap to recompute.

### 7. `menu.c` — an interface without objects

Every page is drawn and handled in one function, and `ui_button` both
draws a button and tells you whether it was tapped. This is called
**immediate mode** UI. There are no widget objects, no callbacks, no
retained tree — for a screen this small it is a fraction of the code.

Note that everything works from buttons *and* from touch. A pointer is
never the only way in.

### 8. `nether.c` — the reactor

A love letter to the old pocket edition, which had no nether dimension.
You build a small structure around a reactor core, light it, and the world
turns hostile for a while.

The lesson here is a design one: a second dimension would need another
75 KiB of voxels that we do not have, so instead the event happens in the
world we already have and costs nothing but this file.

Note the pattern is a **table**, not nested `if`s, so the diagram in the
comment and the check in the code cannot drift apart.

---

## Where the memory went

Measured from the linker output for the console build:

| | KiB |
|---|---|
| Screen, double buffered (SDK) | 77 |
| Voxels, 48 × 48 × 40 at one byte | 75 |
| Rest of the SDK, stacks, buffers | 50 |
| Depth buffer, one byte per 2×2 pixels | 5 |
| Block textures, 25 blocks × 3 faces | 9 |
| Edit journal, 1024 changes | 4 |
| Save buffer | 4 |
| Our arena: mobs, dropped items | 6 |
| Everything else | ~2 |
| **Total** | **~223 of 264** |

If you want to add something large, that table is where you go shopping.
The two obvious savings are storing block textures once instead of three
times for blocks whose faces match, and shrinking the world.

---

## Things to try

Ordered from easy to genuinely hard. All of them are real changes that
teach you something.

1. **Change a number.** `WALK_SPEED` and `GRAVITY` are at the top of
   `player.c`. Make the player float like they are on the moon.
2. **Add a block.** Append to `enum block` in `minevox.h` (at the end, or
   you break existing saves), add a row to the table in `blocks.c`, and
   give it a texture. Fifteen minutes.
3. **Add a recipe.** One row in the `recipes` table in `items.c`.
4. **Make torches.** A block that emits light, is not solid, and can be
   placed on walls. The light list already exists.
5. **A new mob.** A row in `mob_info`, a sprite in `mobs_build_sprites`,
   and it will spawn, wander, fight and drop loot with no other changes.
6. **Ambient occlusion.** Darken a block face where neighbouring blocks
   meet it. It is a lookup of three neighbours per face and it makes an
   enormous visual difference.
7. **Make the renderer twice as fast.** The DDA uses floats on a chip with
   no FPU. Rewrite the inner loop in 16.16 fixed point integers. This is
   the real thing: measure first, and check the picture still matches.
8. **Infinite terrain.** Replace the fixed array with chunks that generate
   as you walk. This is the big one, and it is how the real game works.

---

## A warning about the save format

Block ids are written into save files. If you insert a new block in the
middle of `enum block`, every existing world will find that its stone has
become sand. Always add new blocks at the end.

The same applies to `struct save_header`: change a field and old saves
become unreadable. The magic string `MVX2` at the top exists so that the
game notices and refuses, rather than loading nonsense. If you change the
layout, bump it to `MVX3`.
