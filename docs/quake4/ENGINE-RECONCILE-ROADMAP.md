All ground truth confirmed. The SDK game.vcproj uses `GAME_DLL,Q4SDK` (release config: `NDEBUG,_WINDOWS,_FINAL,GAME_DLL,Q4SDK`), produces `gamex86.dll`, ConfigurationType 2 (DLL). The DOOM3 `__DOOM_DLL__` define is a D3-specific game-source split mechanism not used by the Quake4 SDK game project. I now have everything needed to write the roadmap.

---

# Engine‑Reconciliation Roadmap — DOOM 3 GPL engine → load Quake 4 SDK `gamex86.dll` → boot to main menu

> Scope: get our `c:/code/id/DOOM-3/neo` engine (currently `GAME_API_VERSION 8`, 32‑bit Win32) to load the **unmodified, prebuilt** Quake 4 SDK `gamex86.dll` (`GAME_API_VERSION 37`) via `GetGameAPI`, complete `game->Init(...)`, and reach the **main menu**. Gameplay is out of scope, but every cross‑DLL **vtable layout must be exact even for stubbed methods**.

---

## 1. Strategy recap

- **DLL‑first.** The contract is the **SDK headers** at `c:/code/id/quake4-sdk/source`, because the prebuilt `gamex86.dll` was compiled against them. The DLL makes virtual calls *back into our engine's* concrete interface classes (`idCommonLocal`, `idSysLocal`, `idDeclManagerLocal`, `idRenderSystemLocal`, …). Every one of those classes must present a vtable that is **binary‑identical, slot‑for‑slot**, to the SDK interface header.
- **REF (`_q4ref/Quake4Doom`) is a porting *hint*, not a layout authority.** REF rebuilds *both* engine and game from its own **trimmed** headers (it comments out dozens of pure virtuals: `SetInsideLoad/GetInsideLoad`, the `idCVarHelp` family, `idSys` RAVEN block, `idCmdSystem::ArgCompletion_Models`, `idUserInterface::SetStateVector`, etc.) and **commented out `RV_UNIFIED_ALLOCATOR`** (so REF's `idGame::Init` is `Init(void)`). We **cannot** copy REF's trimming or its `Init(void)` call — the stock DLL's vtables expect the full SDK layout and the **3‑arg allocator `Init`**. Use REF only for *what a method does* and *what a safe stub body looks like*.
- **Build defines that fix every vtable in this corpus** (verified in `quake4-sdk/source/idlib/precompiled.h` lines 32‑78): for the retail `gamex86.dll`, `_RV_MEM_SYS_SUPPORT` **OFF**, `_XENON` **OFF**, `_CONSOLE` **OFF**, `Q4SDK` + `Q4SDK_MD5R` **ON**, `RV_SINGLE_DECL_FILE` **ON**, `RV_BINARYDECLS` **OFF**, `_USE_OPENAL` **ON**, `ENABLE_INTEL_SMP` **ON**, and **`RV_UNIFIED_ALLOCATOR` ON**.
  - ⚠️ **Correction to the input report:** the report claimed `RV_UNIFIED_ALLOCATOR` is on because "`ID_REDIRECT_NEWDELETE` is undefined on Windows." Actual mechanism (precompiled.h:23‑26, 70‑72): `ID_REDIRECT_NEWDELETE` is only defined when **`_DEBUG_MEMORY`** is defined. A retail/`_FINAL` `gamex86.dll` does **not** define `_DEBUG_MEMORY`, so `ID_REDIRECT_NEWDELETE` is off, so the `!_RV_MEM_SYS_SUPPORT && !ID_REDIRECT_NEWDELETE` guard fires and **`RV_UNIFIED_ALLOCATOR` is defined**. **Conclusion unchanged: `idGame::Init` is the 3‑arg `Init(alloc, dealloc, msize)` form** (`Game.h:111`; SDK body `Game_local.cpp:412` calls `Memory::InitAllocator`). Our engine must define `RV_UNIFIED_ALLOCATOR` and call the 3‑arg form.

---

## 2. Build setup — compiling the SDK game as `gamex86.dll` inside our repo

### 2a. The game DLL project
- The SDK ships `c:/code/id/quake4-sdk/source/game.vcproj` (legacy VS2003 format). Verified config:
  - `ConfigurationType="2"` (DLL), `OutputFile="$(ProjectName)x86.dll"` → **`gamex86.dll`**.
  - PreprocessorDefinitions: debug `_DEBUG,_WINDOWS,GAME_DLL,Q4SDK`; **release `NDEBUG,_WINDOWS,_FINAL,GAME_DLL,Q4SDK`**.
- **For boot‑to‑menu, you do NOT need to compile the game.** The simplest path is to **drop the prebuilt retail `gamex86.dll`** (the one shipped with Quake 4 1.4.2 / the SDK's binary) next to the engine and load it. That is the entire point of DLL‑first: the contract is the binary, and rebuilding it from source only matters if you want to debug into the game.
- **If you do build it** (to step into `GetGameAPI`/`Init`): convert `game.vcproj` → a `game.vcxproj` mirroring REF's `c:/code/id/_q4ref/Quake4Doom/src/game.vcxproj` approach — Win32/`v145`, ConfigurationType DynamicLibrary, defines `_WINDOWS;GAME_DLL;Q4SDK` (drop `_FINAL` for a debuggable build but keep `Q4SDK` so `Q4SDK_MD5R` activates and `RV_UNIFIED_ALLOCATOR` stays on). Output `gamex86.dll`.

### 2b. idlib handling
- The SDK game links the **SDK's idlib** (`quake4-sdk/source/idlib.vcproj`), not our D3 idlib. **Keep them separate**: the DLL is self‑contained for idlib symbols. Our engine continues to use **our** idlib. The only idlib types that cross the boundary are POD/value types embedded in interface signatures (`idStr`, `idDict`, `idVec3/4`, `idList`, `idBitMsg`, `idFile`, `idCmdArgs`). Those must be **layout‑compatible** — for boot‑to‑menu the ones actually exchanged on the menu path (`idStr` by value from `idCVarSystem::WriteFlaggedVariables(int)`, `idDict`, `idVec3/4`) are layout‑identical between D3 and Q4 idlib, so no idlib surgery is required for the milestone.

### 2c. `__DOOM_DLL__`
- `__DOOM_DLL__` is a **DOOM 3‑specific** macro that splits D3's game source so the same `.cpp` files compile into *either* the monolithic exe *or* the game DLL. **The Quake 4 SDK game project does not use it** (it defines `GAME_DLL`/`Q4SDK` instead). Since we load the SDK DLL rather than building our own game from D3 sources, `__DOOM_DLL__` is irrelevant to this milestone — leave the engine's existing usage alone.

### 2d. Which SDK interface headers the ENGINE adopts vs. keeps
The engine must change **every header whose class is passed across `gameImport_t`/`gameExport_t`** so the vtable matches. Adopt SDK layout (rewrite our `neo/...` header to the v37 vtable, keep our concrete `*Local` impl):

| Adopt v37 layout (vtable crosses boundary) | Keep as‑is |
|---|---|
| `neo/framework/Common.h`, `CmdSystem.h`, `CVarSystem.h` | `neo/tools/compilers/aas/AASFileManager.h` (already identical) |
| `neo/sys/sys_public.h` (`idSys`) | idlib value types (no change needed for menu) |
| `neo/framework/FileSystem.h`, `DeclManager.h` | `binary.conf` (no vtable impact) |
| `neo/renderer/{RenderSystem.h, RenderWorld.h, ModelManager.h, Model.h, Material.h}` | |
| `neo/ui/{UserInterface.h, ListGUI.h}` | |
| `neo/sound/sound.h`, `neo/cm/CollisionModel.h`, `neo/framework/async/NetworkSystem.h` | |
| `neo/game/Game.h` (`idGame`, `idGameEdit`, structs, `GAME_API_VERSION 37`) | |
| **NEW** `neo/bse/BSEInterface.h` (`rvBSEManager`) | |

---

## 3. Engine interface reconciliation — ORDERED checklist

Menu‑critical interfaces first. Risk = likelihood of a boot‑crash if done wrong (H = wrong slot crashes immediately on the menu path; M = wrong slot crashes only on a less‑traveled but reachable path or has subtle ABI; L = layout‑only, not exercised at menu).

> **Universal rule for every interface below:** add the new virtuals to the `neo/...` *interface header* in **exact SDK slot order**, then add overrides to the concrete `*Local` class in the **same order** with trivial bodies, and keep existing D3 bodies for surviving methods. A single missing/extra/reordered slot mis‑aligns all later calls.

### ① `idGame` / `idGameEdit` — **menu‑critical, risk H** (the whole milestone hinges here)
**Files:** `neo/game/Game.h` (replace `idGame`+`idGameEdit` with `quake4-sdk/source/game/Game.h` lines 103‑371 & 423‑608 verbatim), impl in `neo/game/Game_local.{h,cpp}`.
- **`Init` slot 1 → 3‑arg allocator form** (define `RV_UNIFIED_ALLOCATOR` engine‑side). **H** — slot‑1 mismatch crashes on the first game call. SDK `Init` body calls `common->GetUserCmdHz()`/`GetUserCmdMSec()` → those must exist on `idCommon` (see ②).
- `SetUserInfo` drops `canModify`; **insert `RepeaterSetUserInfo`, `IsClientActive`** (~slots 5‑6) → shifts everything below. **H**.
- `InitFromNewMap`/`InitFromSaveGame` drop `idSoundWorld*`; `SaveGame` gains `saveType_t`; **`RunFrame(cmd,0,true,0)`** 4‑arg; **insert `MenuFrame` (empty stub — menu path), `RepeaterFrame`**. **H** for `MenuFrame` placement (menu loop calls it), **M/L** for the rest.
- Large inserted blocks (all stubbable, **L** for layout): `Repeater*`, effects (`StartViewEffect/PlayEffect`→NULL, `GetCurrentGravity`,`Translation`,`SpawnClientMoveable`), debug‑hud, notetaking/banlist/MP, demo/repeater tail, `GetLoadingGui`/`SetupLoadingGui`.
- `ServerWriteSnapshot` `byte*`→`dword*` + extra arg; `ClientPrediction` gains `ClientStats_t*=NULL`.
- **Keep functional & correctly placed:** `HandleGuiCommands`, `HandleMainMenuCommands`, `StartMenu`, `HandleESC` — these are live menu callbacks. **H**.
- **DELETE OURS‑only D3 methods:** `GetBestGameType`, `GetMapLoadingGUI`, `SelectTimeGroup`, `GetTimeGroupTime` (phantom slots). **H**.
- **`FlushBeforelevelLoad`** is `_RV_MEM_SYS_SUPPORT`‑only → **omit** (stays in sync with DLL).
- `idGameEdit`: slot 1 `ParseSpawnArgsToRenderLight` `void`→`bool`; dozens of inserted ANIM_*/Entity*/Script*/Thread*/Map* methods. **Not called at menu → all trivial stubs, risk L**, but layout must be exact (engine holds the same `gameEdit` pointer the DLL exports).

### ② `idCommon` — **menu‑critical, risk H**
**Files:** `neo/framework/Common.h` (replace `idCommon` body with SDK `framework/Common.h` ~170‑347), impl `neo/framework/Common.cpp` (`idCommonLocal`).
- **Insert at slots 9‑14:** `GetUserCmdHz`(→60), `GetUserCmdMSec`(→16), `GetFrameTime`(→`com_frameTime`), `IsRenderableGameFrame`(→true), `SetRenderableGameFrame`(empty), `GetErrorMessage`(→`errorMessage`). **H** — `GetUserCmdHz/MSec` are called *inside the DLL's `Init`* (`Game_local.cpp:426‑427`), and the 6 insertions shift the entire vtable.
- `InitTool(const int tool, …)`; insert `IsToolActive`(→false), `GetSourceControl`(→NULL).
- `BeginRedirect` gains 4th arg `bool rcon=false` — **ABI‑different, must match**. **H**.
- Insert after `WriteFlaggedCVarsToFile`: `ModViewThink`, `RunAlwaysThinkGUIs(int)`, `DebugcheckBreakpoint`, `DoingDeclValidation`(→false), `SetCrashReportAutoSendString`, `LoadToolsDLL`/`UnloadToolsDLL`.
- **Make existing private `DumpWarnings` PUBLIC+VIRTUAL** in its inserted slot (after `FatalError`). **M**.
- **Localization block REPLACES `GetLanguageDict`** (which is **removed**): add `GetLocalizedString(const char*,int=-1)` (delegate to `languageDict`; returning the token is OK for first boot), `GetLocalizedString(int,int)`→NULL, `GetNumLanguages`→1, `GetNumLocalizedStrings`, `GetLanguage`→"english", `LanguageHasVO`→true. **H** — menu `#str_` lookups go through here and the removal of `GetLanguageDict` shifts slots.
- `KeysFromBinding`/`BindingFromKey`/`ButtonState`/`KeyState` reorder after localization.
- Append `GetRModeForMachineSpec`, `SetDesiredMachineSpec`, `IsRcon`→false.
- Add `saveType_t` enum; forward‑declare `rvISourceControl`, `idInterpreter`, `idProgram`. Keep `InitHeaps`/`ShutdownHeaps` **out** (`_RV_MEM_SYS_SUPPORT` off).

### ③ `idUserInterface` / `idUserInterfaceManager` / `idListGUI` — **menu‑critical, risk H**
**Files:** `neo/ui/UserInterface.h`, `neo/ui/ListGUI.h`, impls `neo/ui/UserInterfaceLocal.h`+`UserInterface.cpp`, `neo/ui/ListGUILocal.h`+`ListGUI.cpp`.
- `idUserInterface`: add `wrapInfo_t` struct; **insert `SetInteractive(bool)`** right after `IsInteractive` (shifts all). After `SetStateFloat`: `SetStateVector`, `SetStateVec4`, `ClearState`(→`state.Clear()`), `DeleteState`(distinct from `DeleteStateVar`), `GetLightColor`(→zero vec4). After `GetStateFloat`: `GetStateVector`, `GetStateVec4`, **`GetDesktop` (promote existing non‑virtual → virtual override, →`desktop`)**. Rename `ReadFromDemoFile`→`ReadFromDemo`, `WriteToDemoFile`→`WriteToDemo` (keep slots). Append `GetMaxTextIndex`(→false). **Do NOT add** `_XENON` `GetMaterialsList` or `_RV_MEM_SYS_SUPPORT` `IsLevelLoadReferenced/SetLevelLoadReferenced`. **H** — these guis are built & drawn the instant the menu loads; `GetDesktop`/`SetInteractive`/`ClearState` must work.
- `idUserInterfaceManager`: **REMOVE `FindDemoGui`**; after `FindGui` insert `GuiIndex`(→-1), `FindGuiByIndex`(→NULL), `ClearGameGuis`(empty). Append `RunAlwaysThinkGUIs(int)`(empty), `RegisterIcon(...)`(empty/real). **Do NOT add `FlushGUIs`** (`_RV_MEM_SYS_SUPPORT`). **H** — `Init`/`FindGui` on the menu path, `RegisterIcon` may fire during entityDef parse.
- `idListGUI`: **slot order unchanged**; only add `bool greyed=false` to `Add` and `Push`. **ABI‑load‑bearing** (the DLL pushes the 3rd arg). **M**.

### ④ `idDeclManager` (+ `idDeclBase`/`idDecl`) — **menu‑critical, risk H**
**Files:** `neo/framework/DeclManager.h`, impl `neo/framework/DeclManager.cpp`.
- **Port `declType_t`** to Quake4 values (insert `DECL_MATERIALTYPE/LIPSYNC/PLAYBACK/EFFECT/CAMERADEF` after `DECL_MODELDEF`; drop `DECL_FX/DECL_PARTICLE`; add `DECL_PLAYER_MODEL`) — **values cross the boundary**. **H**.
- **`idDeclBase`/`idDecl` base‑vtable change** (also owned by `idMaterial`): `Parse` gains `bool noCaching`; insert `GetCompressedLength`, `SetReferencedThisLevel`, `RebuildTextSource`, `Validate`. **H** — every `idMaterial`/decl inherits this.
- Rebuild `idDeclManager` to exact SDK order: **`SetInsideLoad`/`GetInsideLoad` first (after dtor)** — DLL calls these 8×/4× during Init; `RegisterDeclFolderWrapper` (replaces D3 `RegisterDeclFolder`, called 14× at Init); `FindType` gains 4th `bool noCaching=false`; insert `ParseGuides/ShutdownGuides/EvaluateGuide/EvaluateInlineGuide`, `FindTable/TableByIndex` (new vs D3), `FindMaterialType/LipSync/Playback/Effect` + `*ByIndex`, `StartPlaybackRecord/SetPlaybackData/GetPlaybackData/FinishPlayback`, and the **tools tail** `GetNewName/GetDeclTypeName/ListDeclSummary/RemoveDeclFile/Validate/AllocateDecl`. **DO NOT trim like REF** — DLL calls `SetInsideLoad/GetInsideLoad/RegisterDeclFolderWrapper/AllocateDecl/GetNewName`. **H**.
- Stubs: `FindMaterialType/LipSync/Playback/Effect` → `static_cast<…>(FindType(DECL_*,name,makeDefault))`; `AllocateDecl` → allocate via registered allocator (DLL calls it); rest trivial. Forward‑declare `rvDeclMatType/LipSync/Playback/Effect/PlayerModel`, `idDeclTable`.

### ⑤ `idFileSystem` — **menu‑critical, risk M**
**Files:** `neo/framework/FileSystem.h`, impl `neo/framework/FileSystem.cpp` (`idFileSystemLocal`).
- Slots 0‑21 identical. After `WriteFile` insert: `RemoveFile`(+`basePath` default), `RemoveExplicitFile`, **`SetIsFileLoadingAllowed`/`GetIsFileLoadingAllowed`** (DLL calls 8×), asset‑log set (`SetAssetLogName/WriteAssetLog/ClearAssetLog/GetAssetLogName`), **`GetNewFileMemory`**(→`new idFile_Memory()`, DLL calls 2×)/`GetNewFilePermanent`(→NULL). `OpenFileWrite` gains `bool ASCII=false`.
- Signature changes: `SetPureServerChecksums`/`GetPureServerChecksums` (int[`MAX_GAMEPAK_PER_OS`]), `GetOSMask`→`unsigned int`, `ReadFile` timestamp `unsigned*`.
- Replace D3 tail with v37 tail (`RelativeDownloadPathForChecksum`, `ValidateDownloadPak*`, `FindFile` drops `scheduleAddons`, `GetMapDeclIndex`, `GetMapDecl(const char*)` overload, `OSpathToImportPath`, `OpenImportFileRead`, `CopyOSFile`×2, demo header r/w, codepak, `SelectDefaultLanguage`, `ClearAddonList`). **`HasD3XP/RunningD3XP/CopyFile(from,to)/FilenameCompare` GONE.** Omit `_XENON` variants. **M** — `GetNumMaps`/`GetMapDecl`/`ReadFile`/`SetIsFileLoadingAllowed` run during menu population.

### ⑥ `idRenderSystem` — **menu‑critical, risk H**
**Files:** `neo/renderer/RenderSystem.h`, impl `neo/renderer/tr_local.h` + `RenderSystem_init.cpp`/`tr_main.cpp`.
- **Update `glyphInfo_t/fontInfo_t/fontInfoEx_t` to v37 (float metrics + `idMaterial*`)** — `RegisterFont` fills a by‑ref buffer the DLL reads. **H** (menu text).
- Add `ESpecialEffectType` enum + nested `TextureTrackCommand` enum at the same textual positions.
- Vtable (Q4SDK_MD5R on, `_RV_MEM_SYS_SUPPORT`/`_XENON`/`_CONSOLE` off): **`DeferredInit` at slot 1**; `GetValidModes` after `IsOpenGLRunning`; `RemoveAllModelReferences` after `FreeRenderWorld`; **`ExportMD5R`+`CopyPrimBatchTriangles`** (Q4SDK_MD5R); `TrackTextureUsage` before `RegisterFont`; `DrawStretchPic(verts)` **loses** min/max args; `DrawStretchCopy`; `DrawTinyChar`/`DrawTinyStringExt` before `DrawSmallChar`; `BeginFrame(viewDef*,w,h)`+`RenderLightFrustum`+`LightProjectionMatrix`+`ToggleSmpFrame`+3 special‑fx after `BeginFrame`; **`EndFrame` gains 2 defaulted out‑params**; `TakeJPGScreenshot`+`TakeScreenshot(...basePath)`; `CaptureRenderToMemory`; `DebugGraph`/`ShowDebugGraph` tail. `PrintMemInfo(MemInfo*)` (alias `MemInfo`=`MemInfo_t`). Omit `FlushLevelImages` (`_RV_MEM_SYS_SUPPORT`).
- **Implement for real:** `RegisterFont`, `SetColor*`, both `DrawStretchPic`, `DrawSmall/BigChar/StringExt`, `BeginFrame`, `EndFrame`, `GetScreenWidth/Height`, `UploadImage`, `GetCardCaps`, `CropRenderSize`. Everything else stub; forward new `RenderLightFrustum`/`LightProjectionMatrix` virtuals to existing free fns. **H**.

### ⑦ `idMaterial` — **menu‑critical, risk H** (inherits ④'s `idDecl` base)
**Files:** `neo/renderer/Material.h`, impl `Material.cpp`.
- `Parse(text,len,**bool noCaching**)`; add `RebuildTextSource`(inline `{return false;}`) and `Validate` overrides. Add `rvDeclMatType* materialType` member + `GetMaterialType` accessors; friends `rvNewShaderStage/rvGLSLShaderStage`, `idMegaTexture`. **Adopt SDK private member ordering wholesale** (the DLL holds `const idMaterial*` and calls non‑virtual inline accessors compiled into it → member offsets must match). Keep `RV_BINARYDECLS` off (no `Serializable<'IMAT'>` base). **H** — `FindMaterial` returns these and the menu parses+draws them immediately. **Cannot be finished without ④'s `idDecl` base update.**

### ⑧ `idSoundSystem` — **menu‑critical, risk M** (biggest single port)
**Files:** `neo/sound/sound.h`, impl `neo/sound/snd_local.h` (`idSoundSystemLocal`) + `snd_system.cpp`/`snd_world.cpp`.
- The **entire `idSoundWorld` facade folds INTO `idSoundSystem`**, most world methods gain a leading `int worldId`, and ~30 Raven methods append. Strategy (mirrors REF): keep our real `idSoundWorldLocal` internally, add `GetSoundWorldFromId(int)→idSoundWorld*`, implement worldId methods as thin forwarders.
- Inserts: `InitVoiceComms`(slot 5)/`ShutdownVoiceComms`/`Frame`/`ForegroundUpdate`; `AsyncMix` moves up; `AllocSoundWorld`/`SetPlayingSoundWorld`/`GetPlayingSoundWorld` **removed**; `BeginLevelLoad(const char*)`; `SetRenderWorld`; `StopAllSounds(int worldId)`; `AllocSoundEmitter(int worldId)→int`; voice/reverb/cinematic tails.
- **`_USE_OPENAL` is ON** in the SDK → **keep** `IsEAXAvailable`/`GetDeviceName`/`GetDefaultDeviceName` slots. (⚠️ the input report said "REF compiles without `_USE_OPENAL`, omit these" — but precompiled.h:53 shows `_USE_OPENAL` **defined** for the SDK Windows build, so these 3 slots are **present** in the DLL's vtable. **Match the SDK header, include them.**) **M** — get this guard wrong and every slot after it shifts.
- **Forward to real impl:** `SetMute`, `StopAllSounds(worldId)`, `PlaceListener`, `Begin/EndLevelLoad`, `AllocSoundEmitter`, `PlayShaderDirectly` (menu music). Rest stub. Extend `soundShaderParms_t` (+`attenuatedVolume/frequencyShift/wetLevel/dryLevel`) to match struct size.

### ⑨ `idNetworkSystem` — **menu‑critical, risk H** (concrete class)
**Files:** `neo/framework/async/NetworkSystem.h` (+ impl `.cpp`). Note: **concrete** `idNetworkSystem networkSystemLocal` (no `*Local` subclass).
- **Insert `Shutdown` at slot 1** (shifts all); add `bool` params to the three `Server*ReliableMessage*`; insert `Repeater*ReliableMessage*`; **REMOVE `ServerGetClientPrediction`**; append the big Raven tail ending at `GetViewerGUID`.
- **`AddSortFunction(const sortInfo_t&)` is CALLED FROM `idGameLocal::Init()` (`Game_local.cpp:589`) via the vtable, before the menu** → its slot must be byte‑exact or boot crashes. **H**. Add helper types `scannedClient_t/scannedServer_t/sortColumn_t/sortInfo_t`. Give new methods inline `{}`/`return 0` bodies (REF's `NetworkSystem.h` is a directly‑usable slot template — but **trust SDK on `Shutdown`'s slot**, which REF commented out).

### ⑩ `idRenderModelManager` — **menu‑critical, risk M**
**Files:** `neo/renderer/ModelManager.h`, impl `ModelManager.cpp`.
- Insert `Reset` after `Shutdown`; insert the 11‑method tools block (`AllocStaticTriSurf`…`CreateLightDef`/`FreeLightDef` + `CheckModel(idRenderModel*)`) after `CheckModel(const char*)`; append `ListModelSummary`; `PrintMemInfo(MemInfo*)`. Wire tools methods to existing `R_*` free fns in `tr_trisurf.cpp` (add `needSilMultiply` to `CleanupTriangles`). **Keep real:** `Init`, `AllocModel`, `FreeModel`, `FindModel`, `CheckModel(name)`, `DefaultModel`. **M** — `Init`/`FindModel`/`DefaultModel` run at renderer setup.

### ⑪ `idSys` — **menu‑critical, risk H** (largest slot delta)
**Files:** `neo/sys/sys_public.h`, impl `idSysLocal` in `neo/sys/win32/win_main.cpp`.
- **Add `virtual ~idSys()` as slot 0** (D3 had none) — shifts the **entire** vtable. **H**.
- Swap `FPU_EnableExceptions`→`FPU_SetPrecision(int)`. Keep `DLL_*` handles as **`int`** (32‑bit SDK). Add the full RAVEN block: `MapKey`, `AddKeyPress/GetNumKeyPresses/GetKeyPress`, 9 window‑ops (void* handles), `ShowConsole…GrabMouseCursor` (incl. `Milliseconds`), 12 stdio wrappers, `OpenURL/StartProcess` (reposition), `GetGUID`. Reuse D3 bodies for surviving methods; one‑line stubs for the rest. **REF kept D3 layout here → do not crib REF.**

### ⑫ `idRenderWorld` — **risk M** (not menu‑critical, but pointer handed to DLL at Init)
**Files:** `neo/renderer/RenderWorld.h`, impl `tr_local.h`.
- Update `renderEntity_t`/`renderLight_t` structs to v37 layout (DLL fills them); add `renderEffect_t/_s`, `attachedModel_t`, `RF_*` enum, PROC_FILE consts; fwd `rvRenderEffectLocal`, `rvDeclMatType`; add `const rvDeclMatType* materialType` to `modelTrace_t`. Insert the 12‑method block after `GetRenderLight` (`WriteRenderLight`…`RemoveAllModelReferences`); **REMOVE `CheckAreaForPortalSky` and `DebugClearPolygons`**; `RenderScene` gains `renderFlags`; insert `HasSkybox/FindVisibleAreas/RenderPortalFades/WorldToScreen`×2; `GetPortals`/`GetPortal(out)` before by‑value `GetPortal`; `ProcessDemoCommand` gains `portalSkyRenderView`; **rename `DebugClearLines`→`DebugClear`**; `DebugBounds`+`depthTest`; insert `MemorySummary/ShowDebug*/DebugFOV`. All stubs OK (no world at menu). Omit `Flush` (`_RV_MEM_SYS_SUPPORT`). **M** (struct layout) / **L** (methods).

### ⑬ `idRenderModel` — **risk M** (layout only)
**Files:** `neo/renderer/Model.h`, all `neo/renderer/Model_*.cpp` subclasses.
- Add `modelCallback_t callback;` as first data member; **add `virtual void DampenFluidGrid(int,int,float){}` as the FIRST virtual (slot 0, before dtor)** across *all* subclasses — shifts every slot. Make `~idRenderModel` out‑of‑line. Insert `InitEmptyFromArgs` (after `InitEmpty`), `HasCollisionSurface` (after `DepthHack`); `InstantiateDynamicModel` gains `surfMask`; rename `ReadFromDemoFile/WriteToDemoFile`→`ReadFromDemo/WriteToDemo`; append `GetSurfaceMask/SetHasSky/GetHasSky/SetViewEntity`. `Timestamp()`→`unsigned int`. Update `srfTriangles_t` to v37 Q4SDK_MD5R layout. Omit `HasSeparateSilTraceMeshes` (`_MD5R_SUPPORT` off). All new = trivial stubs. **M** — `FindModel`/`DefaultModel` hand `idRenderModel*` to the DLL.

### ⑭ `idCollisionModelManager` — **risk M** (not menu‑critical; pointer handed at Init)
**Files:** `neo/cm/CollisionModel.h`, impl `neo/cm/CollisionModel_local.h` + `.cpp`.
- Add abstract `idCollisionModel` class; extend `contactInfo_t` with **`float separation` + `const rvDeclMatType* materialType`** (SDK is the contract → include both; changes `sizeof(trace_t)` crossing the boundary). Rewrite vtable: prepend `Init`/`Shutdown`; `LoadMap(...,bool forceReload)`; `FreeMap(const char* mapName)`; `LoadModel`→`idCollisionModel*` (+mapName, drop precache); add `ExtractCollisionModel/PreCacheModel/FreeModel/PurgeModels/CompoundTrmFromModel`; `ModelFromTrm` (renamed `SetupTrmModel`); `Translation/Rotation/Contents/Contacts` take `idCollisionModel*`; `DebugOutput`/`DrawModel` gain `idMat3 viewAxis`; `ModelInfo(int)`; add `PrintMemInfo/IsLoaded`. Strategy: thin `idCollisionModel` wrapper holding our `cmHandle_t`. **M** (struct size) / **L** (methods — only on map load).

### ⑮ `idCVarSystem` (+ `idCVar`, `idCVarHelp`) — **menu‑critical, risk M**
**Files:** `neo/framework/CVarSystem.h`, impl `neo/framework/CVarSystem.cpp`.
- Extend `cvarFlags_t` to `BIT(24)` (`CVAR_INFO`…`CVAR_REPEATERINFO`). Add `cvarHelpCategory_t` + **full `idCVarHelp` value class** (auto‑registering ctor) — the DLL instantiates `idCVarHelp` statics that call `cvarSystem->Register(idCVarHelp*)`. **M**.
- Vtable: insert `Register(const idCVarHelp*)` + `GetHelps` after `Register(idCVar*)`; after the `idFile*` `WriteFlaggedVariables` insert the **3 memory overloads** + `ApplyFlaggedVariables` + **`WriteFlaggedVariables(int)→idStr` (returns idStr BY VALUE — exact sret ABI)**. **M**. Add non‑virtual `SetFlag/RemoveFlag` to `idCVar` (vtable‑safe).

### ⑯ `idAASFileManager` — **risk L, no action**
`neo/tools/compilers/aas/AASFileManager.h` is already character‑identical to SDK/REF. Leave as‑is.

---

## 4. `rvBSEManager` stub spec

**Verified** against `quake4-sdk/source/bse/BSEInterface.h:33‑58`: the vtable is **dtor + exactly 15 methods**. **Do NOT add REF's extra `RenderEffect`/`AddTraceModel`/`GetTraceModel`/`FreeTraceModel`** — they corrupt the slot order the DLL expects.

**New files:** `neo/bse/BSEInterface.h` + `neo/bse/BSE_Manager.cpp`.

```cpp
// neo/bse/BSEInterface.h
#ifndef __BSE_INTERFACE_H__
#define __BSE_INTERFACE_H__
// copy effectCategory_t + VIEWEFFECT_* enums verbatim from quake4-sdk/source/bse/BSEInterface.h
class rvRenderEffectLocal;          // fwd only — never dereferenced
struct renderEffect_t;              // fwd only

class rvBSEManager {
public:
    virtual                ~rvBSEManager( void ) {}
    virtual bool            Init( void ) = 0;
    virtual bool            Shutdown( void ) = 0;
    virtual bool            PlayEffect( rvRenderEffectLocal *def, float time ) = 0;
    virtual bool            ServiceEffect( rvRenderEffectLocal *def, float time ) = 0;
    virtual void            StopEffect( rvRenderEffectLocal *def ) = 0;
    virtual void            FreeEffect( rvRenderEffectLocal *def ) = 0;
    virtual float           EffectDuration( const rvRenderEffectLocal *def ) = 0;
    virtual bool            CheckDefForSound( const renderEffect_t *def ) = 0;
    virtual void            BeginLevelLoad( void ) = 0;
    virtual void            EndLevelLoad( void ) = 0;
    virtual void            StartFrame( void ) = 0;
    virtual void            EndFrame( void ) = 0;
    virtual bool            Filtered( const char *name, effectCategory_t category ) = 0;
    virtual void            UpdateRateTimes( void ) = 0;
    virtual bool            CanPlayRateLimited( effectCategory_t category ) = 0;
};
extern rvBSEManager *bse;
#endif
```

```cpp
// neo/bse/BSE_Manager.cpp
#include "../idlib/precompiled.h"
#include "BSEInterface.h"
class rvBSEManagerLocal : public rvBSEManager {
public:
    bool  Init( void )                                         { return true;  }
    bool  Shutdown( void )                                     { return true;  }
    bool  PlayEffect( rvRenderEffectLocal*, float )            { return false; }
    bool  ServiceEffect( rvRenderEffectLocal*, float )         { return true;  } // expired
    void  StopEffect( rvRenderEffectLocal* )                  {}
    void  FreeEffect( rvRenderEffectLocal* )                  {}
    float EffectDuration( const rvRenderEffectLocal* )        { return 0.0f;  }
    bool  CheckDefForSound( const renderEffect_t* )           { return false; }
    void  BeginLevelLoad( void )                              {}
    void  EndLevelLoad( void )                                {}
    void  StartFrame( void )                                  {}
    void  EndFrame( void )                                    {}
    bool  Filtered( const char*, effectCategory_t )           { return true;  } // filter out
    void  UpdateRateTimes( void )                             {}
    bool  CanPlayRateLimited( effectCategory_t )              { return false; }
};
rvBSEManagerLocal bseManagerLocal;
rvBSEManager     *bse = &bseManagerLocal;
```

**Wiring (`neo/framework/Common.cpp`):** add `#include "../bse/BSEInterface.h"`; call `bse->Init();` once during engine init (near `uiManager->Init()`), **before `LoadGameDLL`**; set `gameImport.bse = ::bse;` in the fill block (see §5). **Menu‑critical: none** — no game‑side `bse->` call fires during `Init`/decl‑reg/menu, so the no‑op stub boots.

---

## 5. `gameImport_t` / `gameExport_t` changes

**Verified** against `quake4-sdk/source/game/Game.h:672‑712`. With `_RV_MEM_SYS_SUPPORT` off, `gameImport_t` = `version` + 13 interface pointers + **`rvBSEManager *bse`** (the `heapArena`/`systemHeapArray` members stay `#ifdef`'d out). `gameExport_t` = `version`,`game`,`gameEdit` + **`rvGameLog *gameLog`**.

**`neo/game/Game.h`:**
- `const int GAME_API_VERSION = 37;` (was 8).
- In `gameImport_t`, append after `collisionModelManager`:
  ```cpp
  rvBSEManager *  bse;     // Raven effects system
  // keep heapArena/systemHeapArray ONLY behind #ifdef _RV_MEM_SYS_SUPPORT (never defined)
  ```
- In `gameExport_t`, append after `gameEdit`:
  ```cpp
  rvGameLog *     gameLog; // interface for game logging
  ```
- Add the `rvGameLog` class decl (copy `Game.h:621‑640`) + `extern rvGameLog *gameLog;`. Forward‑declare `rvBSEManager` (or include `bse/BSEInterface.h`).

**`neo/framework/Common.cpp` LoadGameDLL (current fill at lines 2658‑2671):**
- After `gameImport.collisionModelManager = ::collisionModelManager;` add:
  ```cpp
  gameImport.bse = ::bse;
  ```
- The version gate at line 2675 now passes 37 == 37.
- **Replace the `game->Init();` call (line 2689)** with the 3‑arg allocator form — define thunks over the engine heap and pass them:
  ```cpp
  static void * GameAllocThunk( size_t s )  { return Mem_Alloc( (int)s ); }
  static void   GameFreeThunk( void *p )    { Mem_Free( p ); }
  static size_t GameMsizeThunk( void *p )   { return (size_t)_msize( p ); }
  ...
  if ( game != NULL ) {
      game->Init( GameAllocThunk, GameFreeThunk, GameMsizeThunk );
  }
  ```
  (Engine‑side must define `RV_UNIFIED_ALLOCATOR` so `idGame::Init`'s declared signature is the 3‑arg form — otherwise the engine compiles a `Init(void)` declaration that mismatches the DLL's slot‑1 vtable entry.)
- Keep reading only `gameExport.game` and `gameExport.gameEdit`; ignore `gameLog` (the slot must exist so the DLL's write lands in valid memory).

---

## 6. New types to add (consolidated)

| Type | Add to (neo) | REF/SDK source of truth |
|---|---|---|
| `rvBSEManager`, `effectCategory_t`/`EC_*`, `VIEWEFFECT_*` | **new** `neo/bse/BSEInterface.h` | `quake4-sdk/source/bse/BSEInterface.h` (REF: `_q4ref/.../src/bse/BSEInterface.h`, but trim REF's extra methods) |
| `rvGameLog`, `ClientStats_t`, `userOrigin_t`, `demoState_t`, `demoReliableGameMessage_t`, `rvClientEffect` (fwd) | `neo/game/Game.h` (come free with the header swap) | `quake4-sdk/source/game/Game.h` |
| `saveType_t`/`ST_*` | `neo/framework/Common.h` | `quake4-sdk/source/framework/Common.h:16‑22` |
| `rvISourceControl`, `idInterpreter`, `idProgram` (fwd) | `neo/framework/Common.h` | SDK `Common.h` |
| `idLangKeyValue` | already in our idlib Lang code | — |
| `idCVarHelp`, `cvarHelpCategory_t` | `neo/framework/CVarSystem.h` | `quake4-sdk/source/framework/CVarSystem.h:194‑244,374‑413` |
| `wrapInfo_t` | `neo/ui/UserInterface.h` | SDK `ui/UserInterface.h` (top) |
| `rvDeclMatType/LipSync/Playback/Effect/PlayerModel`, `rvDeclPlaybackData`, `rvDeclGuide` (fwd) | `neo/framework/DeclManager.h` | REF `framework/decl{MatType,LipSync,Playback}.h`, `DeclPlayerModel.h`; `rvDeclMatType` def in `quake4-sdk/source/framework/declMatType.h` |
| `declType_t` (Quake4 values) | `neo/framework/DeclManager.h` | SDK `declManager.h` |
| `ESpecialEffectType`, nested `TextureTrackCommand` | `neo/renderer/RenderSystem.h` | SDK `RenderSystem.h:175‑181,272‑276` |
| `renderEffect_t/_s`, `attachedModel_t`, `RF_*`, `rvRenderEffectLocal` (fwd), PROC_FILE consts | `neo/renderer/RenderWorld.h` | SDK `RenderWorld.h` |
| `modelCallback_t`, `fluidImpact_t`, `modelTag_t`, `viewEntity_s` (fwd) | `neo/renderer/Model.h` | SDK `Model.h` |
| `rvNewShaderStage/rvGLSLShaderStage`, `idMegaTexture` (fwd/friends) | `neo/renderer/Material.h` | SDK `Material.h` |
| `rvCommonSample`, `rvSoundShaderEdit`, `soundPortalTrace_t`, `SOUNDWORLD_*` | `neo/sound/sound.h` | SDK `sound/sound.h` |
| `idCollisionModel` (abstract), `rvDeclMatType` (fwd), `WORLD_MODEL_NAME` | `neo/cm/CollisionModel.h` | SDK `cm/CollisionModel.h` |
| `scannedClient_t/scannedServer_t/sortColumn_t/sortInfo_t`, `proxyDownload_t`, `MAX_GAMEPAK_PER_OS` | `neo/framework/async/NetworkSystem.h`, `neo/framework/FileSystem.h` | SDK `async/NetworkSystem.h:16‑62`, `FileSystem.h` |
| `MemInfo` alias = our `MemInfo_t` | idlib `Heap.h` (typedef alias) | SDK uses class `MemInfo` |

---

## 7. First 5 concrete build steps (today → "DLL loads + version 37 accepted")

> Goal of this sub‑milestone: `GetGameAPI` succeeds, `gameExport.version == 37` passes the gate, and `game->Init(alloc,dealloc,msize)` is *callable* (even if it then asserts deeper). This is reachable **before** any menu rendering, and it is the first measurable win.

1. **Define the build flags & bump the API version.** Add `RV_UNIFIED_ALLOCATOR` to the engine build (Win32, matching the DLL). In `neo/game/Game.h` set `GAME_API_VERSION = 37`, append `rvBSEManager *bse;` to `gameImport_t` and `rvGameLog *gameLog;` to `gameExport_t`, add the `rvGameLog` decl, and gate `Init` on `RV_UNIFIED_ALLOCATOR` (3‑arg form). *(This alone makes the version gate pass and forces the correct `Init` signature; nothing links yet because `idGameLocal` is the DLL's.)*

2. **Land the `rvBSEManager` stub** (`neo/bse/BSEInterface.h` + `neo/bse/BSE_Manager.cpp` from §4) and add both to the engine project. Provides the global `::bse` the fill needs.

3. **Wire `Common.cpp::LoadGameDLL`:** `#include "../bse/BSEInterface.h"`, add `gameImport.bse = ::bse;`, add the alloc/free/msize thunks, and change `game->Init()` → `game->Init(GameAllocThunk, GameFreeThunk, GameMsizeThunk)`. Call `bse->Init();` before `LoadGameDLL`. *(Now the engine fills the struct the DLL expects and calls the right Init slot.)*

4. **Make `idCommon` Init‑safe (the two methods the DLL's `Init` calls):** in `neo/framework/Common.h`/`.cpp` add at least `GetUserCmdHz()`→60 and `GetUserCmdMSec()`→16 **in their correct slots 9‑10**. Without these the DLL's `idGameLocal::Init` (which calls `common->GetUserCmdHz()/GetUserCmdMSec()`, `Game_local.cpp:426‑427`) dereferences the wrong slots and crashes. *(Ideally do the full `idCommon` port ② here; minimally these two unblock Init.)*

5. **Make `idNetworkSystem` Init‑safe:** port `neo/framework/async/NetworkSystem.h` to the v37 layout (insert `Shutdown` at slot 1, the bool params, **remove `ServerGetClientPrediction`**, append the tail ending at `GetViewerGUID`) with inline stub bodies, so that **`AddSortFunction`** lands on the exact slot `idGameLocal::Init` calls (`Game_local.cpp:589`). *(This is the other vtable the DLL walks during Init; getting it wrong crashes at boot.)*

After these five, drop the prebuilt `gamex86.dll` next to the engine and run: `GetGameAPI` should return version 37 and `game->Init(...)` should execute through the allocator + the two Init‑time callbacks. Remaining clusters (③ ui, ④ declManager/material, ⑥ renderSystem, ⑧ sound, ⑪ sys) are then the path from "Init returns" to "main menu draws."

---

## 8. Open risks / unknowns

- **H — idlib value‑type ABI across the boundary.** We assume `idStr`/`idDict`/`idList`/`idVec*`/`idBitMsg`/`idFile`/`idCmdArgs` are layout‑compatible between D3 idlib and Q4 idlib for the *menu‑path* signatures. The riskiest is **`idCVarSystem::WriteFlaggedVariables(int)` returning `idStr` by value** (sret) and `idCVarHelp` statics constructed inside the DLL with the DLL's `idStr`. If `idStr` sizes/inline‑buffer differ, this corrupts. **Verify `sizeof(idStr)` and `idDict` match** before trusting the menu path.
- **H — exact slot order is hand‑audited.** Every "insert at slot N" must be confirmed against the SDK header, not the report. Recommend a **vtable‑slot dump / `static_assert(sizeof)`** on each `*Local` class, and ideally a tiny harness that prints each interface's method count vs. the SDK header. A single off‑by‑one (esp. `idSys` dtor slot 0, `idRenderModel` `DampenFluidGrid` slot 0, `idCommon` slots 9‑14, `idNetworkSystem` `Shutdown` slot 1) is an immediate boot crash.
- **M — `_USE_OPENAL` sound‑slot disagreement.** The input report told us to *omit* `IsEAXAvailable`/`GetDeviceName`/`GetDefaultDeviceName` (REF's config). The SDK precompiled.h **defines `_USE_OPENAL`** (line 53), so the **retail DLL includes these 3 slots** — I've flagged §3⑧ to *include* them. **Confirm against the actual shipped DLL** (e.g. by checking whether it imports OpenAL32) before finalizing the sound vtable; guessing wrong shifts ~10 later slots.
- **M — `Q4SDK_MD5R` renderer slots.** `ExportMD5R`/`CopyPrimBatchTriangles` and the `srfTriangles_t`/`newShaderStage_t` size changes depend on `Q4SDK_MD5R` being active in the *specific* `gamex86.dll` we load (the comment at precompiled.h:59‑61 warns only the **Windows** retail build had MD5R; Linux/Mac didn't). We target Windows → MD5R on. Verify the DLL we actually ship with matches.
- **M — allocator thunk correctness.** The DLL calls `Memory::InitAllocator(alloc,dealloc,msize)` and then routes **all** game‑side allocations through our `Mem_Alloc`/`Mem_Free`. If our `Mem_Free` can't free a pointer our `Mem_Alloc` returned without extra bookkeeping, or `_msize` disagrees with our allocator's block layout, the game heap corrupts. May need a dedicated arena for game allocations rather than the engine's tagged `Mem_Alloc`.
- **L/M — `idGameEdit`/`idGame` tail correctness.** ~80 inserted `idGame` virtuals + the entire `idGameEdit` Raven block are stubbed; they're not menu‑exercised, but any *engine* code path that dereferences `gameEdit` (tools, console commands) will walk the new vtable — keep tools disabled for the milestone.
- **Unknown — which exact `gamex86.dll` binary.** The whole layout hinges on the DLL being the **retail 1.4.2 (`GAME_API_VERSION 37`)** build with the defines above. If the available binary is a debug/`_DEBUG_MEMORY` build, `ID_REDIRECT_NEWDELETE` flips on and `RV_UNIFIED_ALLOCATOR` flips **off**, making `Init` the **no‑arg** form — the opposite of our plan. **Confirm the DLL's build flavor first** (the version gate accepting 37 is necessary but not sufficient to prove the allocator flavor).