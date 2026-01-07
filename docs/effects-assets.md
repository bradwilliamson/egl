# Effects asset pipeline (EGL)

Most “modern effects visuals” work can be done **asset-only**: the cgame registers particle/decal materials by filename, and effects code spawns them by selecting a `PT_*` particle type or `DT_*` decal type.

## HD sourcing + upscaling notes (safe + practical)

- Prefer **creating your own** replacement textures (or using packs whose licenses explicitly allow reuse/redistribution).
- If you upscale existing assets, preserve **alpha** and edges:
  - Work in linear space when possible, then re-export to a format EGL loads (`.tga` in these paths).
  - Avoid haloing: use an alpha-aware workflow (premultiply/unpremultiply correctly) and do a small edge clean-up pass.
  - ESRGAN-style upscalers can work well for soft sprites, but you’ll usually need a quick manual pass on thin beams/rings.
- If you pull inspiration from community HD packs, treat them as **reference** unless their license permits direct reuse.

## Where particle/decal textures are defined

- Particle type enum: [cgame/cg_effects.h](../cgame/cg_effects.h)
- Particle + decal → texture registration: `CG_FXMediaInit()` in [cgame/cg_media.c](../cgame/cg_media.c)

At runtime:

- Particles use `cgMedia.particleTable[type]` and optional `cgMedia.particleCoords[type]` (sub-UV sprite sheets).
- Decals use `cgMedia.decalTable[type]` and optional `cgMedia.decalCoords[type]`.

## Common “random pickers” (sprite variants)

Many effects don’t reference a single `PT_*` directly; they call helpers that pick between variants:

- `pRandSmoke()` in [cgame/cg_particles.c](../cgame/cg_particles.c)
  - `PT_SMOKE` → `egl/parts/smoke1.tga`
  - `PT_SMOKE2` → `egl/parts/smoke2.tga`
- `pRandGlowSmoke()` in [cgame/cg_particles.c](../cgame/cg_particles.c)
  - `PT_SMOKEGLOW` → `egl/parts/smoke_glow.tga`
  - `PT_SMOKEGLOW2` → `egl/parts/smoke_glow2.tga`
- `pRandFire()` in [cgame/cg_particles.c](../cgame/cg_particles.c)
  - `PT_FIRE1..PT_FIRE4` → `egl/parts/firetable.tga` (2×2 sprite sheet)
- `pRandEmbers()` in [cgame/cg_particles.c](../cgame/cg_particles.c)
  - `PT_EMBERS1` → `egl/parts/embers1.tga`
  - `PT_EMBERS2` → `egl/parts/embers2.tga`
  - `PT_EMBERS3` → `egl/parts/embers3.tga`

If you replace the files above (same internal paths), all effects that use these helpers will inherit your new look.

## Weapon effects mapping (base weapons)

Notes:

- Impacts/explosions are largely driven by temp entities in [cgame/cg_tempents.c](../cgame/cg_tempents.c).
- Projectile trails are handled in [cgame/cg_parttrail.c](../cgame/cg_parttrail.c) and dispatched from [cgame/cg_entities.c](../cgame/cg_entities.c) based on `EF_*` flags.

### Quick reference table

| Weapon | Primary particle textures | Primary decal textures |
| --- | --- | --- |
| Blaster | `egl/parts/blaster_red.tga`, `egl/parts/smoke_glow*.tga` | `egl/decals/blaster_redmark.tga`, `egl/decals/blaster_burnmark.tga` |
| Hyperblaster | `egl/parts/blaster_blue.tga`, `egl/parts/smoke_glow*.tga` | `egl/decals/blaster_bluemark.tga`, `egl/decals/blaster_burnmark.tga` |
| Shotgun / SSG / MG / Chaingun | `egl/parts/spark.tga`, `egl/parts/smoke*.tga`, `egl/parts/generic.tga` | `egl/decals/bullet.tga` |
| Grenade Launcher | `egl/parts/smoke*.tga` (+ shared explosion set) | shared explosion decals |
| Rocket Launcher | `egl/parts/flare_glow.tga`, `egl/parts/firetable.tga`, `egl/parts/smoke*.tga` (+ shared explosion set) | shared explosion decals |
| Railgun | `egl/parts/rail_core.tga`, `egl/parts/rail_wave.tga`, `egl/parts/rail_spiral.tga` | `egl/decals/rail_*.tga` |
| BFG10K | `egl/parts/bfg_dot.tga`, `egl/parts/flare_glow.tga`, `egl/parts/smoke_glow*.tga` | `egl/decals/bfg_*.tga` |

### Blaster

**Trail**

- `EF_BLASTER` → `CG_BlasterGoldTrail()` in [cgame/cg_parttrail.c](../cgame/cg_parttrail.c)
- Particles:
  - `PT_BLASTER_RED` → `egl/parts/blaster_red.tga`
  - Optional water bubbles: `PT_WATERBUBBLE` → `egl/parts/water_bubble.tga`

**Impact**

- `TE_BLASTER` → `CG_BlasterGoldParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Decals:
  - `DT_BLASTER_REDMARK` → `egl/decals/blaster_redmark.tga`
  - `DT_BLASTER_BURNMARK` → `egl/decals/blaster_burnmark.tga`
- Particles:
  - `PT_BLASTER_RED` → `egl/parts/blaster_red.tga`
  - `pRandGlowSmoke()` → `egl/parts/smoke_glow.tga`, `egl/parts/smoke_glow2.tga`

**High-impact replacement set (6–8 files)**

- `egl/parts/blaster_red.tga`
- `egl/parts/smoke_glow.tga`
- `egl/parts/smoke_glow2.tga`
- `egl/decals/blaster_redmark.tga`
- `egl/decals/blaster_burnmark.tga`
- `egl/parts/water_bubble.tga` (optional but very visible on water maps)

### Hyperblaster

Hyperblaster shots don’t use a dedicated particle trail by default; the most visible “swap-friendly” visuals are the impact particles/decals.

**Impact**

- `TE_BLUEHYPERBLASTER` → `CG_BlasterBlueParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Decals:
  - `DT_BLASTER_BLUEMARK` → `egl/decals/blaster_bluemark.tga`
  - `DT_BLASTER_BURNMARK` → `egl/decals/blaster_burnmark.tga`
- Particles:
  - `PT_BLASTER_BLUE` → `egl/parts/blaster_blue.tga`
  - `pRandGlowSmoke()` → `egl/parts/smoke_glow.tga`, `egl/parts/smoke_glow2.tga`

**High-impact replacement set (6–8 files)**

- `egl/parts/blaster_blue.tga`
- `egl/parts/smoke_glow.tga`
- `egl/parts/smoke_glow2.tga`
- `egl/decals/blaster_bluemark.tga`
- `egl/decals/blaster_burnmark.tga`
- `egl/parts/water_bubble.tga` (optional on water maps)

### Shotgun / Super Shotgun / Machinegun / Chaingun (bullet weapons)

These weapons share the same “bullet hit wall” effect path.

**Impact**

- `TE_GUNSHOT` and `TE_SHOTGUN` → `CG_RicochetEffect()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Decals:
  - `DT_BULLET` → `egl/decals/bullet.tga`
- Particles:
  - `PT_SPARK` → `egl/parts/spark.tga`
  - `pRandSmoke()` → `egl/parts/smoke1.tga`, `egl/parts/smoke2.tga`
  - `PT_GENERIC` → `egl/parts/generic.tga` (small “dots” around the impact)

**High-impact replacement set (6–8 files)**

- `egl/decals/bullet.tga`
- `egl/parts/spark.tga`
- `egl/parts/smoke1.tga`
- `egl/parts/smoke2.tga`
- `egl/parts/generic.tga`
- `egl/parts/generic_glow.tga` (often used as an accent in other effects; helps keep the style consistent)

### Grenade Launcher

**Trail**

- `EF_GRENADE` → `CG_GrenadeTrail()` in [cgame/cg_parttrail.c](../cgame/cg_parttrail.c)
- Particles:
  - `pRandSmoke()` → `egl/parts/smoke1.tga`, `egl/parts/smoke2.tga`
  - Optional bubbles: `PT_WATERBUBBLE` → `egl/parts/water_bubble.tga`

**Explosion / impact**

- `TE_GRENADE_EXPLOSION(_WATER)` → `CG_ExplosionParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Uses the shared textures in “Rocket / grenade / plasma explosions” below.

**High-impact replacement set (6–8 files)**

- `egl/parts/smoke1.tga`
- `egl/parts/smoke2.tga`
- `egl/parts/exploflash.tga`
- `egl/parts/explowave.tga`
- `egl/parts/explo1.tga`
- `egl/parts/explo2.tga`
- `egl/decals/explomark.tga`

### Rocket Launcher

**Trail**

- `EF_ROCKET` → `CG_RocketTrail()` in [cgame/cg_parttrail.c](../cgame/cg_parttrail.c)
- Particles commonly used:
  - `PT_FLAREGLOW` → `egl/parts/flare_glow.tga`
  - `PT_BLUEFIRE` → `egl/parts/bluefire.tga` (when the rocket is in water)
  - Fire variants (picked by `pRandFire()`): `egl/parts/firetable.tga` (2×2 sprite sheet)
  - Smoke variants (picked by `pRandSmoke()`): `egl/parts/smoke1.tga`, `egl/parts/smoke2.tga`
  - Embers variants (picked by `pRandEmbers()`): `egl/parts/embers1.tga`, `egl/parts/embers2.tga`, `egl/parts/embers3.tga`
  - Some trail elements also use `PT_GENERIC` / `PT_GENERIC_GLOW`:
    - `egl/parts/generic.tga`
    - `egl/parts/generic_glow.tga`

**Launcher muzzle smoke (firing)**

- `MZ_ROCKET` in [cgame/cg_muzzleflash.c](../cgame/cg_muzzleflash.c) → `CG_RocketFireParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Particles:
  - `pRandSmoke()` → `egl/parts/smoke1.tga`, `egl/parts/smoke2.tga`

**Explosion / impact**

- `TE_ROCKET_EXPLOSION(_WATER)` → `CG_ExplosionParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Uses the shared textures in “Rocket / grenade / plasma explosions” below.

**High-impact replacement set (6–8 files)**

- `egl/parts/flare_glow.tga`
- `egl/parts/firetable.tga`
- `egl/parts/smoke1.tga`
- `egl/parts/smoke2.tga`
- `egl/parts/exploflash.tga`
- `egl/parts/explowave.tga`
- `egl/decals/explomark.tga`

### Railgun

Railgun visuals are driven by `CG_RailTrail()` in [cgame/cg_parttrail.c](../cgame/cg_parttrail.c) and triggered by `TE_RAILTRAIL` in [cgame/cg_tempents.c](../cgame/cg_tempents.c).

**Particles**

- Beam core: `PT_RAIL_CORE` → `egl/parts/rail_core.tga`
- Impact “wave” sprite: `PT_RAIL_WAVE` → `egl/parts/rail_wave.tga`
- Optional spiral trail: `PT_RAIL_SPIRAL` → `egl/parts/rail_spiral.tga`
- Center “spots” along the beam: `PT_GENERIC_GLOW` → `egl/parts/generic_glow.tga`

**Decals (impact marks)**

- `DT_RAIL_WHITE` → `egl/decals/rail_white.tga`
- `DT_RAIL_BURNMARK` → `egl/decals/rail_burnmark.tga`
- `DT_RAIL_GLOWMARK` → `egl/decals/rail_glowmark.tga`

**High-impact replacement set (6–8 files)**

- `egl/parts/rail_core.tga`
- `egl/parts/rail_wave.tga`
- `egl/parts/rail_spiral.tga`
- `egl/parts/generic_glow.tga`
- `egl/decals/rail_white.tga`
- `egl/decals/rail_burnmark.tga`
- `egl/decals/rail_glowmark.tga`

### BFG10K

**Projectile trail**

- `EF_BFG` → `CG_BfgTrail()` in [cgame/cg_parttrail.c](../cgame/cg_parttrail.c)
- Particles:
  - `PT_BFG_DOT` → `egl/parts/bfg_dot.tga`
  - `PT_FLAREGLOW` → `egl/parts/flare_glow.tga`

**Explosion (small) / “feet zap”**

- `TE_BFG_EXPLOSION` → `CG_ExplosionBFGEffect()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Particles:
  - `PT_BFG_DOT` → `egl/parts/bfg_dot.tga`
  - `pRandGlowSmoke()` → `egl/parts/smoke_glow.tga`, `egl/parts/smoke_glow2.tga`

**Explosion (big)**

- `TE_BFG_BIGEXPLOSION` → `CG_ExplosionBFGParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c)
- Particles:
  - `PT_BFG_DOT` → `egl/parts/bfg_dot.tga`
  - `pRandGlowSmoke()` → `egl/parts/smoke_glow.tga`, `egl/parts/smoke_glow2.tga`
- Decals:
  - `DT_BFG_BURNMARK` → `egl/decals/bfg_burnmark.tga`
  - `DT_BFG_GLOWMARK` → `egl/decals/bfg_glowmark.tga`

**High-impact replacement set (6–8 files)**

- `egl/parts/bfg_dot.tga`
- `egl/parts/flare_glow.tga`
- `egl/parts/smoke_glow.tga`
- `egl/parts/smoke_glow2.tga`
- `egl/decals/bfg_burnmark.tga`
- `egl/decals/bfg_glowmark.tga`
- `egl/parts/exploflash.tga` (shows up strongly in the larger blast sequences)

## Modernization guide (assets + code knobs)

### Scaling knobs (global + per-family)

EGL has a global `r_effectscale` (default `1.0`; `0` auto-scales based on resolution). This repo also supports per-family multipliers:

- `r_effectscale_blaster` (PT_BLASTER_*)
- `r_effectscale_rail` (PT_RAIL_*)
- `r_effectscale_bfg` (PT_BFG_DOT)
- `r_effectscale_ion` (PT_IONTAIL/PT_IONTIP)
- `r_effectscale_phalanx` (PT_PHALANXTIP + `sprites/s_photon.sp2`)
- Shared buckets used across weapons:
  - `r_effectscale_smoke` (smoke/fire/flare variants)
  - `r_effectscale_explo` (PT_EXPLO* + flash/wave)
  - `r_effectscale_spark` (PT_SPARK)

Practical starting points at 1440p/4K (adjust to taste):

- `set r_effectscale 0` (auto)
- `set r_effectscale_rail 1.2`
- `set r_effectscale_bfg 1.1`
- `set r_effectscale_explo 1.15`
- `set r_effectscale_smoke 0.95`

### Glow/bloom notes (what’s feasible here)

This renderer’s “program” system is **ARB vertex/fragment program** based (not GLSL). That means:

- You can add **emissive-looking** sprites/particles by authoring brighter textures and/or using additive blend materials.
- True **bloom** is typically a post-process pass; it’s not a small drop-in unless you already have a post chain.

If you want to experiment with shader-like tweaks anyway, look at material passes that reference `fragProgName`/`vertProgName` in the material parser in [renderer/rf_material.c](../renderer/rf_material.c) and the ARB program loader in [renderer/rf_program.c](../renderer/rf_program.c).

One pragmatic “emissive pass” experiment (not true bloom) that fits this renderer:

- Add a new ARB fragment program (e.g. `programs/egl_glow.fp`) that outputs `srcColor * glowIntensity`.
- Add a second material pass for chosen particle/beam materials that uses additive blending and references that fragment program.
- Make `r_glow` a runtime scalar by feeding `glowIntensity` as an ARB program env/local parameter; when `r_glow 0`, the pass effectively becomes a no-op (still a draw call, but minimal risk).

This repo implements a minimal version of that idea:

- `r_glow` (CVAR_ARCHIVE) controls glow intensity in the range **0..2.0** (0 disables).
- Family multipliers (CVAR_ARCHIVE): `r_glow_rail`, `r_glow_explo`, `r_glow_spark`, `r_glow_bfg`, `r_glow_ion` (each clamped 0..2, multiplied with `r_glow`).
- Optional auto boost: `r_glow_autoscale` (CVAR_ARCHIVE, default 0). When enabled **and** `r_effectscale 0` (auto), the renderer multiplies the final glow intensity by the same resolution factor used for effect auto-scale: $\sqrt{(w\cdot h)/(1024\cdot 768)}$.
- The renderer sets ARB fragment program `program.local[9].rgb` to the effective intensity and **skips the emissive pass entirely when the effective intensity is 0**.
- Scope: the glow pass is only applied for **batched particle materials** (materials sorted as particles), so it won’t affect world geometry.

Examples:

- Rail trails: `set r_glow 1.2; set r_glow_rail 1.0`
- Stronger explosions without changing rail: `set r_glow 1.0; set r_glow_explo 1.6; set r_glow_rail 0.8`
- Brighter BFG dot/trail look: `set r_glow 1.4; set r_glow_bfg 1.5`
- Subtle ion bolts/trails: `set r_glow 1.0; set r_glow_ion 1.2`
- Phalanx tip tuning (uses ion family): `give phalanx; set r_glow 1.0; set r_glow_ion 1.3`

Notes on filenames:

- BFG “dot” particle is registered as `egl/parts/bfg_dot.tga` in [cgame/cg_media.c](../cgame/cg_media.c).
- Smoke puffs used broadly in trails/muzzle effects are `egl/parts/smoke1.tga` and `egl/parts/smoke2.tga`.
- Ion (hyperblaster/phalanx) particles are registered as `egl/parts/iontip.tga`, `egl/parts/iontail.tga`, and `egl/parts/phalanxtip.tga` in [cgame/cg_media.c](../cgame/cg_media.c).

Perf caveat:

- When enabled, this adds an extra **additive pass** on a small set of particle textures plus a fragment program bind; monitor with `r_speeds 1` and keep an eye on overdraw at high particle density.

Batching knob:

- `r_max_batch_particles` (CVAR_ARCHIVE, default 4096) caps the *vertex budget* used for a single backend batch of particle-sorted quads (useful for stress testing and tuning batching behavior). Suggested stress value: `8192`.

### Performance notes (more particles without tanking FPS)

- The biggest real cost is usually **overdraw** (large translucent quads), not spawning.
- Prefer fewer, sharper elements (rings/cores) over huge soft clouds.
- Useful knobs:
  - `cg_particleMax` (cap)
  - `cg_particleCulling` (reduces off-screen/behind-camera work)
  - `cg_particleSmokeLinger` (keeps smoke around longer; can raise total count)

### Concrete test commands

Use these to quickly iterate on effects assets/scale:

- `map base1`
- `give all`
- `god`
- `noclip`
- `use weapon_railgun` / `use weapon_rocketlauncher` / `use weapon_bfg`
- `set r_effectscale 0`
- `set r_effectscale_rail 1.3`
- `set r_effectscale_explo 1.2`

### Per-family weapon test commands (1080p vs 4K)

Resolution in EGL is controlled by the latched cvars `vid_width`/`vid_height` (apply with `vid_restart`).

| Family | 1080p setup | 4K setup | Fire/test | Scale knobs to tweak |
| --- | --- | --- | --- | --- |
| Blaster / Hyperblaster | `set vid_width 1920; set vid_height 1080; vid_restart` | `set vid_width 3840; set vid_height 2160; vid_restart` | `sv_cheats 1; give all; use weapon_blaster` | `set r_effectscale 0; set r_effectscale_blaster 1.0; set r_effectscale_smoke 1.0` |
| Bullet weapons (SG/SSG/MG/Chaingun) | `set vid_width 1920; set vid_height 1080; vid_restart` | `set vid_width 3840; set vid_height 2160; vid_restart` | `sv_cheats 1; give all; use weapon_chaingun` | `set r_effectscale 0; set r_effectscale_spark 1.0; set r_effectscale_smoke 1.0` |
| Grenades / Rockets (shared explosions) | `set vid_width 1920; set vid_height 1080; vid_restart` | `set vid_width 3840; set vid_height 2160; vid_restart` | `sv_cheats 1; give all; use weapon_rocketlauncher` | `set r_effectscale 0; set r_effectscale_explo 1.0; set r_effectscale_smoke 1.0` |
| Railgun | `set vid_width 1920; set vid_height 1080; vid_restart` | `set vid_width 3840; set vid_height 2160; vid_restart` | `sv_cheats 1; give all; use weapon_railgun` | `set r_effectscale 0; set r_effectscale_rail 1.0` |
| BFG | `set vid_width 1920; set vid_height 1080; vid_restart` | `set vid_width 3840; set vid_height 2160; vid_restart` | `sv_cheats 1; give all; use weapon_bfg` | `set r_effectscale 0; set r_effectscale_bfg 1.0; set r_effectscale_explo 1.0` |

Perf/debug quick-checks while iterating:

- `set cl_showfps 1` (HUD FPS)
- `timerefresh` (scene benchmark)
- `set r_speeds 1` (renderer stats)
- `set gl_showtris 1` (wire/tri visualization)
- `set r_debugBatching 1` (batching diagnostics)

## Rocket / grenade / plasma explosions (shared)

Rocket/grenade/plasma explosions are driven by `CG_ExplosionParticles()` in [cgame/cg_parteffects.c](../cgame/cg_parteffects.c).

| Asset | Particle/decal type(s) | Texture |
| --- | --- | --- |
| Explosion frames | `PT_EXPLO1..PT_EXPLO7` | `egl/parts/explo1.tga` .. `egl/parts/explo7.tga` |
| Flash | `PT_EXPLOFLASH` | `egl/parts/exploflash.tga` |
| Shockwave | `PT_EXPLOWAVE` | `egl/parts/explowave.tga` |
| Sparks | `PT_SPARK` | `egl/parts/spark.tga` |
| Smoke | `PT_SMOKE`, `PT_SMOKE2` | `egl/parts/smoke1.tga`, `egl/parts/smoke2.tga` |
| Explosion embers | `PT_EXPLOEMBERS1`, `PT_EXPLOEMBERS2` | `egl/parts/exploembers.tga`, `egl/parts/exploembers2.tga` |
| Burn decals | `DT_EXPLOMARK..DT_EXPLOMARK3` | `egl/decals/explomark*.tga` |

**Particles**

- Animated explosion frames:
  - `PT_EXPLO1..PT_EXPLO7` → `egl/parts/explo1.tga` .. `egl/parts/explo7.tga`
- Explosion flash + shockwave:
  - `PT_EXPLOFLASH` → `egl/parts/exploflash.tga`
  - `PT_EXPLOWAVE` → `egl/parts/explowave.tga`
- Sparks:
  - `PT_SPARK` → `egl/parts/spark.tga`
- Smoke:
  - `PT_SMOKE` → `egl/parts/smoke1.tga`
  - `PT_SMOKE2` → `egl/parts/smoke2.tga`
- Explosion “embers” sprites:
  - `PT_EXPLOEMBERS1` → `egl/parts/exploembers.tga`
  - `PT_EXPLOEMBERS2` → `egl/parts/exploembers2.tga`

**Decals (burn marks)**

- `DT_EXPLOMARK..DT_EXPLOMARK3` → `egl/decals/explomark.tga`, `egl/decals/explomark2.tga`, `egl/decals/explomark3.tga`

If you want a “modern” explosion look with minimal work: start by replacing `exploflash.tga`, `explowave.tga`, and the `explo*.tga` sequence.

## Modernization guide (per weapon)

This section is intentionally “asset-first”: it focuses on what you can do by replacing textures at the registered paths.

General advice that applies to all weapon effects:

- Prefer smooth alpha gradients (avoid hard edges) to reduce shimmering and harsh sprite outlines.
- Keep the alpha footprint tight (don’t make the entire texture 10–20% opaque) to reduce overdraw cost.
- If you change sprite sheets (`firetable.tga`), keep the same 2×2 layout so the sub-UV logic still lines up.
- Use `r_effectscale` to keep effect sizes feeling right across resolutions.

### Blaster

- **Hi-res focus**: make `blaster_red.tga` read as a bright, small, hot “energy fleck” (tight core, soft halo).
- **Scaling**: if impacts feel too big on high res, try `r_effectscale 0.85` or `0` (auto).
- **Performance**: the trail is many small quads; keep the sprite compact and avoid huge soft halos.

### Hyperblaster

- **Hi-res focus**: `blaster_blue.tga` + `smoke_glow*.tga` dominate the look.
- **Style direction**: consider a less “puffy” glow-smoke (more defined bright core, less mid-alpha fog).

### Bullet weapons (SG/SSG/MG/Chaingun)

- **Hi-res focus**: `bullet.tga` and `spark.tga` are the big levers.
- **Style options**: make `spark.tga` either thin + sharp (metal ricochet) or chunkier + streaky (tracer-like), but keep alpha sparse.

### Grenade Launcher

- **Hi-res focus**: `smoke1.tga`/`smoke2.tga` set the trail quality; explosions are shared with rockets.
- **Style options**: modern trails usually look best with higher-frequency detail but a soft, fast-fading edge.

### Rocket Launcher

- **Hi-res focus**: `flare_glow.tga` (core), `firetable.tga` (heat), and smoke/embers textures.
- **Sprite sheet caution**: `firetable.tga` is a 2×2 sheet; keep all four frames and consistent padding.

### Railgun

- **Hi-res focus**: `rail_core.tga` + `rail_wave.tga` plus the three decals.
- **Style options**: a thin core with a subtle outer bloom (baked into the texture) reads “modern” without increasing particle counts.

### BFG10K

- **Hi-res focus**: `bfg_dot.tga` and the BFG decals.
- **Performance**: the BFG trail is bright and layered; keep the dot texture’s edge falloff quick to reduce overdraw.

## Quick test checklist

Suggested (cheat) flow to quickly see each weapon’s effects:

1. `sv_cheats 1`
2. `give all`
3. Use a flat wall + shoot repeatedly.
4. Jump into water and retest rockets/grenades (water bubbles + underwater fire behavior).
5. Tune size with `r_effectscale`.

### Integrated glow test table

Use a flat wall in `dm3` or `base2` and fire repeatedly; swap 1080p/4K with `vid_width`/`vid_height` + `vid_restart`.

| Family | Key CVars | Test commands | Expected outcome |
| --- | --- | --- | --- |
| Hyperblaster/Phalanx (Ion) | `r_effectscale 0`, `r_effectscale_ion 1.5`, `r_glow 1.0`, `r_glow_ion 1.2` | `sv_cheats 1; give all; use weapon_hyperblaster; set r_effectscale 0; set r_effectscale_ion 1.5; set r_glow 1.0; set r_glow_ion 1.2; set vid_width 3840; set vid_height 2160; vid_restart; impulse 101; r_speeds 1` | Blue bolts/trails (`iontip`/`iontail`) are scaled larger with mild glow; toggle `r_glow 0` to confirm the emissive pass is skipped. |
| Railgun | `r_effectscale 0`, `r_effectscale_rail 2.0`, `r_glow 1.5` | `sv_cheats 1; give all; use weapon_railgun; set r_effectscale 0; set r_effectscale_rail 2.0; set r_glow 1.5; set vid_width 3840; set vid_height 2160; vid_restart` | Rail trail sprites scale thicker and emissive pass brightens without changing world lighting; verify `r_speeds 1` stays stable (batched particles, no explosion in draw calls beyond the extra glow pass). |
| Rocket/Grenade (shared explosions) | `r_effectscale 0`, `r_effectscale_explo 1.8`, `r_effectscale_smoke 2.0`, `r_glow 1.2` | `sv_cheats 1; give all; use weapon_rocketlauncher; set r_effectscale 0; set r_effectscale_explo 1.8; set r_effectscale_smoke 2.0; set r_glow 1.2; set cl_showfps 1; timerefresh` | Explosions look chunkier with subtle additive glow; smoke trails denser; FPS remains healthy during repeated blasts; check `r_speeds 1` for fill/overdraw sensitivity. |
| BFG | `r_effectscale 0`, `r_effectscale_bfg 3.0`, `r_effectscale_explo 1.5`, `r_glow 1.8` | `sv_cheats 1; give all; use weapon_bfg; set r_effectscale 0; set r_effectscale_bfg 3.0; set r_effectscale_explo 1.5; set r_glow 1.8; set r_debugBatching 1` | BFG dots/trails become massive and glow strongly; shared explosions enhanced; batching debug stays sane (no unexpected unbatching). |
| BFG (tuning) | `r_glow 1.8`, `r_glow_bfg 1.2` | `sv_cheats 1; give all; use weapon_bfg; set r_glow 1.8; set r_glow_bfg 1.2; set r_debugBatching 1` | BFG dot uses a separate multiplier for fine-tuning green emissive without over-brightening rail/explosions. |
| Smoke (shared) | `r_effectscale 0`, `r_effectscale_smoke 2.0`, `r_glow 1.0`, `r_glow_explo 0.8` | `sv_cheats 1; give all; use weapon_rocketlauncher; set r_effectscale 0; set r_effectscale_smoke 2.0; set r_glow 1.0; set r_glow_explo 0.8` | Smoke puffs add a subtle hazy glow (tied to explo multiplier); verify in bright/dark areas and watch `r_speeds 1` for overdraw sensitivity. |
| Blaster/Hyperblaster | `r_effectscale 0`, `r_effectscale_blaster 1.5`, `r_effectscale_spark 2.0`, `r_glow 1.0` | `sv_cheats 1; give all; use weapon_hyperblaster; set r_effectscale 0; set r_effectscale_blaster 1.5; set r_effectscale_spark 2.0; set r_glow 1.0; set vid_width 1920; set vid_height 1080; vid_restart` | Bolts/sparks are larger with mild glow; swap to 4K and verify auto-scale still looks consistent and doesn’t blow out highlights. |

## How to ship overrides (recommended)

Put your replacements in an addon pack that loads after `egl.pkz`.

A simple approach that avoids relying on pack filename ordering:

1. Create a folder like:

   - `baseq2/addons/egl_fx_hd/egl/parts/*.tga`

2. Zip the *contents* of `baseq2/addons/egl_fx_hd/` into a `.pkz` and place it in the same `addons` folder:

   - `baseq2/addons/egl_fx_hd.pkz`

Because EGL scans `*/*.pkz` (subfolders) after `*.pkz` in the root, putting your pack under `addons/` tends to make it override root packs.

For quick iteration, this repo also includes an editable placeholder scaffold under:

- `baseq2/addons/egl/parts/*.tga`
- `baseq2/addons/egl/decals/*.tga`

## Quick build helper (optional)

If you use the helper script in [tools/build-pkz.ps1](../tools/build-pkz.ps1), you can build an addon `.pkz` from a source folder and drop it into `baseq2/addons/`.

On Windows (PowerShell 5+), from the repo root:

- `powershell -NoProfile -ExecutionPolicy Bypass -File tools\build-pkz.ps1 -SourceDir baseq2\addons\egl -OutputPkz baseq2\addons\egl_glow.pkz`

Notes:

- PowerShell’s `Compress-Archive` only supports `.zip`, so the script writes a `.zip` and then renames it to `.pkz` for you.
- Cross-platform alternative: use a zip tool (e.g. `7z`) to zip the *contents* of your source folder, then rename the resulting `.zip` to `.pkz`.

Test:

- Drop the resulting `.pkz` into `baseq2/addons/` and launch EGL; if you use a dedicated addons game dir in your setup, start with `+set fs_game addons`.

## Debugging tips

- If you want to confirm which pack is supplying an asset, enable filesystem developer logging (look for a cvar like `fs_developer`) and watch for `FS_OpenFileRead: pkz file ...` messages when loading the texture.
- If a replacement doesn’t show up, double-check the internal path in the zip matches exactly (forward slashes, correct case where applicable).
