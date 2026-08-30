# Sonic Generations DS

Direct port of **Sonic Generations (Nintendo 3DS)** to **Nintendo DSi**

Assets, level data and parameters come straight from the 3DS RomFS dump
(TARGET.cpk unpacked: BCRES models, CTPK textures, ACB/AWB audio, .bprm params,
AMB stage archives) - converted by build-time tooling in this repo.

Target: DSi (133 MHz ARM9, 16 MB RAM.
Toolchain: devkitARM + libnds (hardware 3D engine, fixed point).

See SPECS.md for the full hardware constraints sheet.
