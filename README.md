# ComS 327 – Pokémon Battle Engine & Related Projects (Spring 2024)

A semester-long series of C/C++ assignments building up to a full terminal-based Pokémon battle engine, culminating in a from-scratch implementation featuring procedural Pokémon generation from real game data, a custom Fibonacci heap, and Dijkstra's-algorithm-based NPC pathfinding.

## Where to Look

- **[`327_assn1.09`](./327_assn1.09)** — the final, complete project. A full Pokémon battle system: wild and trainer battles, move selection, items (potions, revives, pokeballs), catching mechanics, and pokémon swapping — all built on top of a custom Fibonacci heap (`heap.c`) driving Dijkstra's-algorithm pathfinding for NPC movement across a procedurally generated world. Pokémon are generated at runtime from real parsed game data (stats, movesets, species) rather than hardcoded values.
- **[`327_assn1.10`](./327_assn1.10)** — a self-directed bonus project: a cake decorator game built with ncurses, letting the user build and customize a multi-layer cake with different flavors, decorations, and toppers. A lighter, more creative departure from the rest of the semester.
- **`327_assn1.01`–`.08`** — incremental weekly assignments building up to the final project (world generation, character movement, pathfinding foundations, etc.). Kept for completeness, not essential viewing.

## Note on Academic Integrity
If you are a current student, it is against university policy to copy this code for your own assignments.

## Build Instructions
Call `make` then `./Assignment1_xx` where `xx` is the assignment number. Use `make clean` or `make clobber` to remove build artifacts when done.
