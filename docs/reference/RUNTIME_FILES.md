# Runtime files

Type: reference · Audience: players and developers · Status: current
Verified against: `55868979`

What the game reads from the VPK and reads or writes on the memory card.

## In the VPK (`app0:`)

| Path | Read by | Content |
|---|---|---|
| `voxel/` | `src/platform/voxel/` | Voxel world data ([ASSETS.md](ASSETS.md)) |
| `ui/` | `src/platform/vita/vita_stage.c` | Intro leaves margins |
| `ui2/` | `src/platform/ui/ui_assets.c` | The new UI's fixed atlas |
| `followers/followers.pak` | `src/follower_mon.c` (`vita_pak.c`) | Follower sprites and messages (PKF1 container) |

## On the memory card (`ux0:data/POKEVITA/`)

| File | Written by | When | Content |
|---|---|---|---|
| `saves/pokeemerald*.sav` | `sdl2.c` | Always | Saves, 128 KB GBA flash layout; slot 1–3 (`VitaControls_GetSavePath`) |
| `pokeemerald.cfg` | `sdl2.c` `StoreConfigFile` | Always | Platform settings (filter, buttons, 3D on/off, framerate…) |
| `settings.txt` | `src/platform/vita/vita_extras.c` | Always | Extras, graphics and voxel options (`VitaExtras_GetFast`) |
| `followers/followers.pak` | — (read by `vita_pak.c`) | Read only | Looked for after `app0:`, so a pack can be installed without rebuilding the VPK |
| `thread_violations.log` | `src/platform/vita/vita_game_thread.c` | Only on a violation | Game state changed off the game thread, with the caller's address ([../guides/DEBUGGING.md](../guides/DEBUGGING.md)) |
| `debug.log` | `VITA_LOG` | Builds with `VITA_LOGGING` | Boot and error log |
| `perf_summary.txt`, `perf_frames.csv` | `src/platform/vita/vita_perf.c` | Builds with `VITA_TELEMETRY` | Per-frame telemetry |
| `ui_stall.log` | `src/platform/ui/ui_stall.c` | Builds with `VITA_TELEMETRY` | New UI stalls over 100 ms, and which core held the frame |

A crash leaves `ux0:data/psp2core-*.psp2dmp`, written by the system
([../guides/DEBUGGING.md](../guides/DEBUGGING.md), "Crash dumps").
