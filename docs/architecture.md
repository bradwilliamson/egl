# EGL Engine Architecture

High-level map of the EGL Quake II engine. **Every claim cites folder + files + functions.** Where evidence is missing, that is stated.

---

## Overview

EGL uses a **client–server model** with **loadable game modules** (DLLs/SOs) and **platform abstraction**. The codebase splits into:

- **`common/`** — Shared utilities (networking, filesystem, console, cvars, collision). Used by both client and server.
- **`client/`** — Client networking, input, sound, GUI. Links to renderer.
- **`server/`** — Server networking, game state, physics. Loads the game module.
- **`renderer/`** — OpenGL rendering (frontend `rf_*`, backend `rb_*`).
- **`win32/`**, **`unix/`**, **`sdl2/`** — Platform-specific code (entry, GL, input, sound, net).
- **`cgame/`**, **`game/`** — Game logic; built as separate modules and loaded at runtime.

*Evidence: layout matches repo structure; `common/common.c` uses `Com_Init`, `Com_Frame`, `Com_Shutdown`; `Makefile` lines 46–58 list `COMMON_SRC`, `CLIENT_SRC`, `SERVER_SRC`, `RENDERER_SRC`, `CGAME_SRC`, `GAME_SRC`.*

---

## 1. Client / server separation

### 1.1 Main loop and frame entry points

- **Common** drives the main loop. It calls **server** then **client** each frame.
- **Evidence:** `common/common.c`: `Com_Frame()` (line 851) invokes `SV_Frame(msec)` (879) then `CL_Frame(msec)` (884) when not dedicated. `Com_Init()` (658) calls `SV_ServerInit()` (809) and `CL_ClientInit()` (814).

- **Client frame:** `client/cl_main.c` defines `CL_Frame()` (1365), `CL_ClientInit()` (2231), `CL_ClientShutdown()` (102). `CL_Frame` uses `CL_ReadPackets()` (1269), `CL_SendCmd()` (called from refresh path), and triggers rendering via `SCR_UpdateScreen()` (1465, 1505).

- **Server frame:** `server/sv_main.c` defines `SV_Frame()` (905), `SV_ServerInit()` (970), `SV_ServerShutdown()` (1061). `SV_Frame` calls `SV_ReadPackets()` (761), `SV_RunGameFrame()` (874), and `SV_SendClientMessages()` (517 in `server/sv_send.c`).

### 1.2 Client state machine

- Client connection states: `CA_UNINITIALIZED` → `CA_DISCONNECTED` → `CA_CONNECTING` → `CA_CONNECTED` → `CA_ACTIVE`. (`CA_LOADING` is not present in the enum; the doc previously mentioned it by analogy.)
- **Evidence:** `shared/shared.h` (346–350): `caState_t` enum `CA_UNINITIALIZED`, `CA_DISCONNECTED`, `CA_CONNECTING`, `CA_CONNECTED`, `CA_ACTIVE`. `client/cl_main.c` uses them (e.g. 142, 454–460, 507–509, 596–604, 1217–1239, 1414–1514).

### 1.3 Server state machine

- Server states: `SS_DEAD` → `SS_LOADING` → `SS_GAME` (and `SS_CINEMATIC`).
- **Evidence:** `shared/shared.h` (357–360): `ssState_t` with `SS_DEAD`, `SS_LOADING`, `SS_GAME`, `SS_CINEMATIC`. `server/sv_init.c`: `SV_SetState(SS_LOADING)` (106, 197), `SV_SetState(SS_DEAD)` (146), `SV_SpawnServer(..., SS_GAME, ...)` (399), `SS_CINEMATIC` (382). `server/sv_main.c`: `SV_ServerShutdown` sets `Com_SetServerState(SS_DEAD)` (1081).

### 1.4 Network protocol and transport

- **Transport:** UDP. Send/receive are platform-specific; common code uses abstract `NET_*` APIs.
- **Evidence:** `win32/win_sock.c`: `NET_SendPacket()` (389), `NET_GetPacket()` (327). `unix/unix_udp.c`: same (316, 272). `common/net_chan.c` calls `NET_SendPacket` (122, 287). `client/cl_main.c` uses `NET_GetPacket(NS_CLIENT, ...)` (1271); `server/sv_main.c` uses `NET_GetPacket(NS_SERVER, ...)` (767).

- **Client→server commands** use `CLC_*` enums. **Evidence:** `common/protocol.h` (43–48): `CLC_BAD`, `CLC_NOP`, `CLC_MOVE`, `CLC_USERINFO`, `CLC_STRINGCMD`, `CLC_SETTING`. Client sends these via `net_chan` / message building (e.g. `CL_ForwardCmdToServer` in `client/cl_main.c` 136–149 writes `CLC_STRINGCMD`).

- **Server→client messages** use `SVC_*` enums. **Evidence:** `shared/shared.h` (277–317): `SVC_BAD`, `SVC_NOP`, `SVC_DISCONNECT`, `SVC_SERVERDATA`, `SVC_CONFIGSTRING`, `SVC_FRAME`, `SVC_PACKETENTITIES`, etc. Parsing in `client/cl_parse.c`: `CL_ParseServerMessage()` (1074) switches on `SVC_*` (1162–1319). Server writes them in e.g. `server/sv_ents.c` (`SVC_FRAME`, `SVC_PACKETENTITIES`, `SVC_PLAYERINFO`), `server/sv_user.c` (`SVC_SERVERDATA`, `SVC_CONFIGSTRING`, …), `server/sv_send.c` (`SVC_PRINT`, `SVC_SOUND`, …).

### 1.5 Where client and server live

- **Client:** `client/cl_main.c`, `cl_parse.c`, `cl_input.c`, `cl_screen.c`, plus GUI (`gui_*.c`), server browser (`cl_serverbrowser.c`), etc. **Evidence:** `client/cl_main.c` (CL_*), `client/cl_parse.c` (`CL_ParseServerMessage`), `client/cl_input.c` (`CL_SendCmd` 635), `client/cl_screen.c` (`SCR_UpdateScreen` 81).

- **Server:** `server/sv_main.c`, `sv_send.c`, `sv_ents.c`, `sv_user.c`, `sv_init.c`, `sv_gameapi.c`, etc. **Evidence:** `server/sv_main.c` (SV_Frame, SV_ReadPackets, SV_ServerInit, SV_ServerShutdown), `server/sv_send.c` (`SV_SendClientMessages` 520), `server/sv_ents.c` (SVC_* writes), `server/sv_gameapi.c` (game API, `ge->RunFrame` 546).

- **Shared networking / messaging:** `common/net_chan.c`, `common/net_msg.c`, `common/protocol.h`. **Evidence:** `net_chan.c` uses `NET_SendPacket`, sequence numbers, etc.; `protocol.h` defines `CLC_*` and protocol constants.

---

## 2. Renderer flow

### 2.1 Invocation from client

- Rendering is triggered from the **client** during the frame. The **cgame** module produces the view; the **renderer** draws it.
- **Evidence:** `client/cl_screen.c`: `SCR_UpdateScreen()` (81) calls `R_BeginFrame()` (119), then `CL_CGModule_RenderView(separation[i])` (135), then `R_EndFrame()` (142). `client/cl_main.c`: `CL_Frame` calls `SCR_UpdateScreen` (1465, 1505). `client/cl_cgapi.c`: `CL_CGModule_RenderView()` (266) calls `cge->RenderView(...)` (269).

### 2.2 CGame → Renderer API

- CGame submits entities, polygons, etc. via the **R_*** API. The client passes these function pointers into the cgame module.
- **Evidence:** `client/cl_cgapi.c`: `cgi.R_AddEntity = R_AddEntity`, `cgi.R_AddPoly = R_AddPoly`, `cgi.R_BeginFrame = R_BeginFrame`, `cgi.R_EndFrame = R_EndFrame` (736–737, 767–768). CGame uses them, e.g. `cgame/cg_entities.c` (`cgi.R_AddEntity`), `cgame/cg_view.c` (`cgi.R_BeginFrame`, `cgi.R_EndFrame`, `cgi.R_AddEntity`, `cgi.R_AddPoly`), `cgame/cg_tempents.c`, `cgame/cg_weapon.c`, etc.

### 2.3 Renderer frontend (`rf_*`)

- **`renderer/rf_main.c`:** Central entry points. `R_BeginFrame()` (353), `R_EndFrame()` (392), `R_AddEntity()` (522), `R_AddPoly()` (542). Aggregates polys via `R_AddPolysToList()` (44) and feeds the backend.
- **`renderer/rf_world.c`:** BSP world. Uses `R_AddMeshToList()` and surfaces.
- **`renderer/rf_alias.c`:** Model (alias) rendering. `R_AddMeshToList`, then `RB_PushMesh` (584).
- **`renderer/rf_light.c`:** Lighting logic (used by world/entities).
- **`renderer/rf_decal.c`:** Decals. `R_AddMeshToList` (91), `RB_PushMesh` (119).
- **`renderer/rf_2d.c`:** 2D draw. `RB_PushMesh` (94).
- **`renderer/rf_font.c`:** Font rendering.
- **`renderer/rf_sky.c`:** Sky. `R_AddMeshToList` (232), `RB_PushMesh` (322).
- **`renderer/rf_meshbuffer.c`:** `R_AddMeshToList()` (47), `RB_PushMesh` (349, 389).
- **`renderer/rf_sprite.c`:** Sprites. `R_AddMeshToList` (82), `RB_PushMesh` (177, 262).

**Evidence:** `renderer/rf_main.c`, `rf_world.c`, `rf_alias.c`, `rf_decal.c`, `rf_2d.c`, `rf_meshbuffer.c`, `rf_sky.c`, `rf_sprite.c` as above; `r_public.h` declares `R_DrawPic`, `R_DrawFill`, `R_CullBox`, `R_RegisterFont`, `R_DrawString`, etc.

### 2.4 Renderer backend (`rb_*`)

- **`renderer/rb_batch.c`:** `RB_PushMesh()` (162) batches geometry.
- **`renderer/rb_render.c`:** OpenGL state and draw orchestration (uses `Sys_Milliseconds` 2478).
- **`renderer/rb_state.c`:** State caching.
- **`renderer/rb_qgl.c`:** OpenGL loader (Windows `LoadLibrary` / Unix `dlopen`) and `qgl*` wrappers.

**Evidence:** `renderer/rb_batch.c` (159–162), `rb_render.c`, `rb_state.c`, `rb_qgl.c`; `rf_*` call `RB_PushMesh` as cited.

### 2.5 Platform OpenGL

- **Windows:** `win32/win_glimp.c` — `GLimp_Init()` (624). **Evidence:** `win32/win_glimp.c` 617–624.
- **Unix:** `unix/unix_glimp.c` — `VID_Init()` (336), `GLimp_Init()` (400). **Evidence:** `unix/unix_glimp.c` 333–336, 397–400.
- **SDL2:** `sdl2/sdl_glimp.c` — `GLimp_Init()` (103), `VID_Init()` (336). **Evidence:** `sdl2/sdl_glimp.c` 103, 336.

- The renderer uses platform GL only indirectly (via `rb_qgl` / `GLimp_*`). **Evidence:** `renderer/rf_init.c` calls `GLimp_Init()` (1397).

---

## 3. Platform abstractions

### 3.1 System

- **`Sys_Init`**, **`Sys_Quit`**, **`Sys_Milliseconds`**, **`Sys_LoadLibrary`** are implemented per platform.
- **Evidence:** `win32/win_main.c`: `Sys_Init` (169), `Sys_Quit` (195), `Sys_Milliseconds` (316), `Sys_LoadLibrary` (675). `unix/unix_main.c`: same (254, 266, 347, 676). `common/common.c` calls `Sys_Init` (714); `Com_Quit` → `Sys_Quit` (400). `win32/win_main.c` main loop uses `Sys_Milliseconds` (826, 846); `unix/unix_main.c` (869, 873).

### 3.2 Video and OpenGL

- **`VID_Init`**, **`VID_Shutdown`** (and **`GLimp_Init`** / **`GLimp_Shutdown`**) are platform-specific.
- **Evidence:** `win32/win_vid.c`: `VID_Init` (562). `unix/unix_glimp.c`: `VID_Init` (336), `GLimp_Init` (400). `sdl2/sdl_glimp.c`: `VID_Init` (336), `GLimp_Init` (103). `client/cl_main.c` calls `VID_Init` (2249). `CL_ClientShutdown` (102) calls `VID_Shutdown` (124) via shutdown path.

### 3.3 Input

- **`IN_Init`**, **`IN_Shutdown`** (and **`IN_Frame`** where used) are platform-specific.
- **Evidence:** `win32/win_input.c`: `IN_Init` (1079). `unix/x11_main.c`: `IN_Init` (797). `sdl2/sdl_input.c`: `IN_Init` (893). `client/cl_main.c` calls `IN_Init` (2261); `CL_ClientShutdown` calls `IN_Shutdown` (123).

### 3.4 Sound

- **`Snd_Init`**, **`Snd_Shutdown`** are in the client; backend is platform- or library-specific.
- **Evidence:** `client/snd_main.c`: `Snd_Init` (868), `Snd_Shutdown`; uses `ALSnd_Init` (OpenAL) or `DMASnd_Init` (platform). `client/cl_main.c` shutdown calls `Snd_Shutdown` (121). `win32/win_vid.c` calls `Snd_Init` (520); `unix_glimp.c` references init; `sdl_glimp.c` calls `Snd_Init` (314).

### 3.5 Network

- **`NET_Init`**, **`NET_SendPacket`**, **`NET_GetPacket`** are implemented per platform.
- **Evidence:** `win32/win_sock.c`: `NET_Init` (730), `NET_GetPacket` (327), `NET_SendPacket` (389). `unix/unix_udp.c`: same (589, 272, 316). `common/common.c` calls `NET_Init` (806); `Com_Shutdown` calls `NET_Shutdown` (894).

### 3.6 Platform layout

- **`win32/`:** `win_main.c` (entry, `Sys_*`), `win_glimp.c` (GL), `win_vid.c` (window, `VID_*`), `win_input.c` (input), `win_snd.c`, `win_sock.c` (net), `win_console.c`.
- **`unix/`:** `unix_main.c` (entry, `Sys_*`, `Sys_LoadLibrary`), `unix_glimp.c` (GL, `VID_*`), `x11_main.c` (window, input), `x11_utils.c`, `unix_udp.c` (net), `unix_snd_*.c` (ALSA/OSS/SDL), `unix_console.c`.
- **`sdl2/`:** `sdl_main.c` (SDL init/shutdown hooks), `sdl_glimp.c` (GL, video), `sdl_input.c`, `sdl_snd.c`.

**Evidence:** File lists in `win32/`, `unix/`, `sdl2/`; `Makefile` (47–52) uses `win32` or SDL2 sources for client; `Makefile.unix` uses `unix` (and optionally SDL2).

---

## 4. Module loading (game DLLs)

### 4.1 Who loads what

- **CGame** is loaded by the **client**; **Game** is loaded by the **server**.
- **Evidence:** `client/cl_cgapi.c`: `Sys_LoadLibrary(LIB_CGAME, &cgi)` (791). `server/sv_gameapi.c`: `Sys_LoadLibrary(LIB_GAME, &gi)` (530).

### 4.2 Platform loaders

- **Windows:** `LoadLibrary` for `eglcgamex64.dll` / `gamex64.dll`. **Evidence:** `win32/win_main.c` `Sys_LoadLibrary` (675) uses `LoadLibrary` (699, 707, 721); `sys_libList` (638–639) references `eglcgame`/`game` DLLs and `GetCGameAPI`/`GetGameAPI`.
- **Unix:** `dlopen` for `eglcgame.so` / `game.so`. **Evidence:** `unix/unix_main.c` `Sys_LoadLibrary` (676) uses `dlopen` (736, 752); same `sys_libList` layout (642–643).

### 4.3 Module exports

- **CGame** exposes **`GetCGameAPI`**; **Game** exposes **`GetGameAPI`**.
- **Evidence:** `cgame/cg_api.c`: `GetCGameAPI` (41) returns `cgExport_t*`. `game/g_main.c`: `GetGameAPI` (111) returns `gameExport_t*`. `win32/win_main.c` (638–639) and `unix/unix_main.c` (642–643) list these symbol names.

### 4.4 CGame / Game API usage

- CGame implements **`RenderView`**, **`ParseServerMessage`**, etc.; Game implements **`RunFrame`**, **`ClientConnect`**, etc.
- **Evidence:** `cgame/cg_api.c`: `cge.RenderView = V_RenderView`, `cge.ParseServerMessage = CG_ParseServerMessage` (76, 67). `cgame/cg_view.c`: `V_RenderView` (474). `cgame/cg_parse.c`: `CG_ParseServerMessage` (147). `client/cl_cgapi.c` checks `cge->RenderView`, `cge->ParseServerMessage`, etc. (806, 805). `server/sv_gameapi.c` checks `ge->RunFrame`, `ge->ClientThink`, etc. (546); `server/sv_main.c` `SV_RunGameFrame` calls `ge->RunFrame()` (888); `server/sv_init.c` also calls `ge->RunFrame` (108, 203–204).

---

## 5. Data flow (with references)

### 5.1 Client frame

```
Com_Frame (common/common.c 851)
  → SV_Frame (879)
  → CL_Frame (884)
       → CL_ReadPackets (cl_main.c 1352) — uses NET_GetPacket (1271)
       → CL_ParseServerMessage (cl_parse.c 1074) — dispatches SVC_*
       → SCR_UpdateScreen (cl_main.c 1465, 1505)
            → R_BeginFrame (cl_screen.c 119)
            → CL_CGModule_RenderView (cl_screen.c 135)
                 → cge->RenderView (cl_cgapi.c 269)
            → R_EndFrame (cl_screen.c 142)
       → CL_SendCmd (via refresh path) — client → server commands
```

### 5.2 Server frame

```
Com_Frame (common/common.c 851)
  → SV_Frame (sv_main.c 905)
       → SV_ReadPackets (924) — NET_GetPacket (767)
       → SV_RunGameFrame (945)
            → ge->RunFrame (sv_main.c 888)
       → SV_SendClientMessages (948, sv_send.c 520)
```

### 5.3 Render frame

```
SCR_UpdateScreen (cl_screen.c 81)
  → R_BeginFrame (rf_main.c 353)
  → cge->RenderView → R_AddEntity / R_AddPoly / etc.
  → R_EndFrame (rf_main.c 392)
       → R_AddPolysToList (rf_main.c 44, 301)
       → … R_AddMeshToList (rf_meshbuffer.c 47), RB_PushMesh (rb_batch.c 162)
       → backend draw
```

---

## 6. Build and dedicated server

- **Windows:** `Makefile` builds `egl.exe`, `eglcgamex64.dll`, `gamex64.dll`; uses `win32` or, with `USE_SDL2=1`, `sdl2` for client. **Evidence:** `Makefile` 46–58, 79, SDL2 branch.
- **Unix:** `Makefile.unix` builds `egl`, `eglcgame.so`, `game.so`; uses `unix` (and optionally SDL2). **Evidence:** `Makefile.unix` structure.
- **Dedicated server:** Built with **`DEDICATED_ONLY`**. No client, no renderer; uses minimal platform (e.g. `win32/win_main.c`, `win_console.c`, `win_sock.c`). **Evidence:** `Makefile` 59–61 (`DED_SYS_SRC`), 73–75 (`DED_*_OBJ`); `common/common.c` uses `#ifdef DEDICATED_ONLY` to skip client init and `CL_Frame` (704–716, 881–884); `server/sv_gameapi.c` (457) and `win32/win_main.c`, `unix/unix_main.c` use `DEDICATED_ONLY` to exclude client/GL.

---

## 7. Gaps / uncited claims

- **`CA_LOADING`:** Not in `shared.h` enum; only `CA_UNINITIALIZED` … `CA_ACTIVE` are defined. Removed from this doc.
- **Exact `CL_Frame` → `CL_SendCmd` call path:** `CL_SendCmd` is used in the input/refresh path; the precise call chain from `CL_Frame` via `CL_RefreshInputs` / etc. was not fully traced here. The roles of `CL_SendCmd` and `CL_ReadPackets` are correctly attributed.
- **Vulkan/Metal:** Not present in tree; "future Vulkan/Metal" is speculative and not claimed as implemented.

---

## 8. Quick reference

| Layer        | Folder      | Key files (examples)                    | Key functions (examples)           |
|-------------|-------------|-----------------------------------------|------------------------------------|
| Common      | `common/`   | `common.c`, `net_chan.c`, `protocol.h`  | `Com_Init`, `Com_Frame`, `Com_Shutdown` |
| Client      | `client/`   | `cl_main.c`, `cl_screen.c`, `cl_parse.c`| `CL_Frame`, `SCR_UpdateScreen`, `CL_ParseServerMessage` |
| Server      | `server/`   | `sv_main.c`, `sv_send.c`, `sv_ents.c`   | `SV_Frame`, `SV_SendClientMessages`, `SV_RunGameFrame` |
| Renderer FE | `renderer/` | `rf_main.c`, `rf_world.c`, `rf_alias.c` | `R_BeginFrame`, `R_EndFrame`, `R_AddEntity`, `R_AddPoly` |
| Renderer BE | `renderer/` | `rb_batch.c`, `rb_render.c`, `rb_qgl.c` | `RB_PushMesh`                      |
| Platform    | `win32/`    | `win_main.c`, `win_glimp.c`, `win_sock.c` | `Sys_Init`, `Sys_LoadLibrary`, `NET_GetPacket` |
| Platform    | `unix/`     | `unix_main.c`, `unix_glimp.c`, `unix_udp.c` | `Sys_Init`, `Sys_LoadLibrary`, `NET_GetPacket` |
| CGame       | `cgame/`    | `cg_api.c`, `cg_view.c`, `cg_parse.c`   | `GetCGameAPI`, `V_RenderView`, `CG_ParseServerMessage` |
| Game        | `game/`     | `g_main.c`                              | `GetGameAPI`, `ge->RunFrame`       |
