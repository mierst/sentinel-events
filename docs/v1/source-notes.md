# Admission source investigation

Read-only inspection, 2026-09-05. Local reference: `_dayz-vanilla-scripts` in the
workspace; it is an extracted tree without Git commit metadata. Record hashes
and the actual installed game/server build when executing the prototype. Source
line numbers below refer to this inspected snapshot, not all DayZ releases.

Official upstream reference: [Bohemia DayZ Script Diff](https://github.com/BohemiaInteractive/DayZ-Script-Diff).
No third-party implementation has been copied into this repository.

| Surface | Inspected vanilla path and location | Evidence and limit |
| --- | --- | --- |
| Disconnect | `scripts/5_Mission/mission/missionServer.c:677` | `PlayerDisconnected` invokes disconnect at 690, saves at 697, then exits Hive. Identity can already be deleted. Cache stable identity earlier. |
| Player disconnect | `scripts/4_World/Entities/ManBase/PlayerBase.c:7320` | `void OnDisconnect()` precedes the vanilla disconnect save; preserve `super`. It is not a crash hook. |
| Connect | `scripts/5_Mission/mission/missionServer.c:422`; `PlayerBase.c:7296` | `InvokeOnConnect` calls `void OnConnect()`. Candidate recovery entry, not proof all interaction is blocked until it finishes. Inspect ready/new/reconnect paths in engine. |
| Vehicle occupancy | `scripts/4_World/Entities/DayZPlayerImplement.c:465` | `IsInVehicle()` accounts for command/transport parent; test transitions and passengers. |
| Teleport root | `scripts/4_World/Plugins/PluginBase/PluginDeveloper/DeveloperTeleport.c:85` | Helper explicitly selects `GetCommand_Vehicle().GetTransport()`. This explains vehicle transport by that helper; not a guarantee that every `SetPosition` call has identical semantics. |
| Inventory removal | `scripts/4_World/Entities/ManBase/PlayerBase.c:6654` | `ClearInventory()` is server guarded, enumerates/deletes items and clears hands. Verify nested content, timing, and persistence in engine. |
| Item creation | `scripts/4_World/Classes/PlayerGearSpawn/CfgPlayerSpawnHandler.c:173,282` | Vanilla uses hands/attachment/cargo creation APIs. Check actual returned entities and inventory; no assumed ground fallback. |
| Protection | `scripts/3_Game/Entities/Object.c:1184` | Native `SetAllowDamage`/`GetAllowDamage`; protects target, does not itself prevent outgoing damage. Preserve preexisting protection state. |
| Input | `scripts/3_Game/gameplay.c:795,907`; `scripts/5_Mission/mission/missionGameplay.c:797` | Mission UI input exclusions exist; `SetInputSuppression` is developer conditional. Neither establishes a general production server freeze guarantee. |
| Failed controller guard | `scripts/3_Game/human.c:7-28` | Server-only disable/speed/angle overrides failed the live movement trial and were withdrawn. See [guard evidence](evidence/guard-and-spawn.md). Do not reuse this as a freeze. |
| Scripted stationary command candidate | `scripts/3_Game/human.c:1202-1302,1585-1593`; `scripts/4_World/Entities/DayZPlayerImplement.c:2367-2403` | Explicit command/physics callbacks and native finish path exist. Synchronized player authority, collision behavior, and restoration remain unproven; see [experiment contract](stationary-guard-experiment.md). |
| Character markers | `scripts/4_World/Entities/ManBase/PlayerBase.c:7034,7087` | `OnStoreSave`/`OnStoreLoad` are serialization surfaces. Versioned appended token/receipt needs save compatibility testing and recovery evidence. |
| JSON | `scripts/3_Game/tools/JsonFileLoader.c:7,42` | `LoadFile`/`SaveFile` return status; writes use OpenFile WRITE, FPrint, CloseFile. No character-save transaction or explicit disk flush guarantee. |

The lead directly inspected disconnect ordering, inventory removal, connect,
JSON writes, and teleport helper behavior. The teleport helper was also checked
against the [official upstream source](https://raw.githubusercontent.com/BohemiaInteractive/DayZ-Script-Diff/main/scripts/4_world/plugins/pluginbase/plugindeveloper/developerteleport.c).
Native behavior and mod compatibility remain prototype questions.

Required first-slice evidence: both fresh-spawn and saved-character reconnect;
driver/passenger/seat transition; client input suppression bypass; incoming and
outgoing damage; nested inventory deletion; generation file interruption;
character save versus journal interruption; and recovery before player interaction.
