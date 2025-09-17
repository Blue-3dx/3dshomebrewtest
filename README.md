Wave (Geometry Dash - Wave Mode) - Starter project template
-----------------------------------------------------------
What's included:
- src/main.c         : Minimal citro2d-based prototype (movement, pipes, collision)
- gfx/wave.png       : Your player icon (copied from uploaded image if available)
- romfs/             : Build-time converted .t3x files will be placed here by the Makefile rule
- Makefile           : Builds the project and converts gfx/*.png -> romfs/*.t3x using tex3ds
- .github/workflows/3ds.yml : GitHub Actions workflow to build on push (uses devkitPro)

Quick notes:
- Edit gfx/wave.png (replace) — on build the PNG will be converted to romfs/wave.t3x
  by the Makefile rule using tex3ds (RGBA8888).
- Version 1 intentionally contains no audio.
- The C code uses citro2d APIs. You'll need devkitPro with citro2d installed to compile.

To build locally (on a machine with devkitPro toolchain):
  make

On GitHub Actions the provided workflow will trigger builds on push to main.

Have fun — push to your repo and the CI will produce the .3dsx artifact (if setup correctly)!
