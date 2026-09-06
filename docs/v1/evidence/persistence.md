# Persistence probe evidence

Status: non-destructive implementation; destructive admission/replay remains disabled.
Recorded 2026-09-05 (America/New_York), native server 1.29.163709, mod 0.0.1-pre.
No inventory removal, item issuance, teleport, quarantine, or automatic recovery
action is implemented by this probe. A separate ignored diagnostic mission issued
an explicit character-save request for the live marker test described below.

## Implemented storage contract

`SevSessionRecord` schema 1 carries the specified sequence, run/player/token,
phase, origin/orientation, mutation/return receipts and return position. Auxiliary
`Integrity` and `PreviousIntegrity` fields detect accidental corruption and chain
retained generations. They are not cryptographic authentication.

`SevSessionStore.WriteManifest(manifest, out error)` creates an immutable run
manifest before session writes. The manifest has schema/run identity and 1-8
unique intended player identities. `WriteNext(record, out error)` creates only a
new `gN.json`; it refuses an existing run/session/file conflict and never overwrites
a verified generation. The caller supplies the next sequence. The store fills
integrity fields, writes, reads back, parses and validates the candidate chain.
`LoadLatest(runId, playerId, out record, out error)` validates every manifest
member before returning the selected record. Any failure clears the output and
returns a bounded error code. A missing member, gap, unexpected generation name,
newer corrupt file, identity/token change, regressing phase, changed origin, or
broken chain blocks loading. Initial member writes are allowed while the manifest
is incomplete; later generations require the complete manifest to load.

Bounds and deliberate limitations:

- Each file is at most 16,384 ASCII bytes, read with a 16,385-byte limit before
  parsing. Records accept only the exact compact JSON produced by this schema's
  serializer; missing, duplicate, extra or reformatted fields fail canonical
  readback. Truncated root envelopes are rejected before native parsing.
- Each identifier is 1-64 ASCII characters. Run/token/receipt alphabets are letters,
  digits, underscore and hyphen. Player IDs additionally allow base64 `+`, `/`,
  `=`. Run path segments use lowercase two-digit hex per input byte. New manifests
  require schema 2 and use session folders `p0` through `p7`, derived only from
  exact, case-sensitive identity position in the validated immutable manifest.
  Record identity is checked on read. Legacy schema-1 manifests retain their hex
  session paths for read/append; there is no migration or layout fallback. Record
  and character-marker schemas remain 1. Unsupported identities or filesystem
  path limits fail closed; raw IDs never become path segments.
- Each session retains 1-32 contiguous generations. Enumeration stops after an
  unexpected entry or the cap. Exhaustion blocks writes; there is no automatic
  pruning, run adoption or restart-time directory crawl. No global retention
  policy is implemented yet.
- All positions must be finite within +/-100,000 per axis; origin and return
  position cannot be zero. Orientation components are bounded to +/-360.
  These are serialization sanity checks, not terrain/safe-placement validation.
- Supported phases are INTENT_RECORDED, MUTATING, PREPARED, RETURNING, RETURNED.
  The first generation is intent. Prepared/returning require a mutation receipt;
  returned requires both receipts. Intent/mutating contain no completion receipts.
  Receipt presence in JSON does not prove that any inventory or Hive action ran.
- The checksum covers canonical fields and previous checksum. It detects accidental
  changes; an administrator can rewrite it. Deleting a complete newest suffix or
  restoring an entire old filesystem snapshot cannot be detected without an
  independent high-water/durability witness. Never authorize destructive replay
  from successful journal loading alone.
- Script write/readback is not fsync, atomic rename, a transaction, or Hive save
  acknowledgement. Filesystem and character persistence remain separate.

## Recovery and character probe

`SevRecovery.Decide(recordValid, tokensMatch, mutationProven, returnProven,
untouchedProven)` returns BLOCK for invalid/mismatched, missing or contradictory
proof. Positive no-mutation proof returns UNTOUCHED; proven mutation without return
returns NEEDS_RETURN; proven mutation plus return returns DONE. No caller in this
slice treats those results as permission to mutate a player.

The sole PlayerBase hook block is in `scripts/4_World/SevPlayerBase.c`. Native
signatures `OnStoreSave(ParamsWriteContext ctx)`,
`OnStoreLoad(ParamsReadContext ctx, int version)`, and the no-argument
OnConnect/OnReconnect/OnDisconnect methods preserve `super`. Connection diagnostics
cache authenticated `PlayerIdentity.GetId()` and log only lifecycle and presence
booleans. IDs, names, tokens and receipts are omitted from mod diagnostics.

An appended canonical envelope holds a versioned run/player/token, explicit probe
state and mutation/return receipts. Invalid/missing optional markers preserve a
successful vanilla load and never mean UNTOUCHED. A failing vanilla load remains
false. Correlation is only against a known requested session and cached stable ID;
no unrelated player is quarantined. Appended-context compatibility with legacy
characters and other mods is NOT VERIFIED: a stream has no script-level rewind,
and reading another mod's following field may consume it. Marker string length is
checked before JSON parsing, but the engine's context read allocates the string
before script can check length.

`SevPersistenceProbe.ENABLED` is a compile-time constant, false in normal builds.
For an isolated diagnostic build only, change it to true before building; choose
`FAULT_POINT` as `before-generation-write`, `partial-generation`, or
`after-generation-readback` to return a controlled write failure. The partial case
leaves a malformed new file while retaining prior generations. No production RPC
or runtime setting exposes fault control. Character save/load and before/after
return-receipt boundaries log when enabled; these logs are observation points,
not save acknowledgements or pause barriers. Server-only
`SevSetDiagnosticMarker(record, state)` installs a simulated marker on a matching
identity when diagnostics are enabled. It never requests a save or mutates items.
Simulated MUTATED/RETURNED markers must not be reported as actual inventory proof.

## Native runs and checks

Source baseline: `fc1f4c515dfb32cd4203ae9deaecefdb8324e23d`; persistence changes
are the commit containing this evidence. Build tool: MakePbo 2.16 / DePbo DLL 10.21.
Server executable SHA-256:
`16CC3EE2A79AD726D0F4609824090F837BCD391FBA7B1A60213184E61CC0D9D9`.
Only Sentinel Events and vanilla were loaded. No client participated in these boots.

- Native Recovery RED: BLOCK stub compiled, six BLOCK fixtures passed, and
  UNTOUCHED/NEEDS_RETURN/DONE failed as expected. Admission 11/11 passed.
  PBO SHA-256 `59E760522E9BADFDA90E6BA149F3AA7412E3011537854DD2B6D0BE690516CEDF`;
  log SHA-256 `0E644060D776A95B55918FBBE7378AF32CC2984BD408953B08DC2903096A4E97`.
- Native Store RED: false stubs compiled; eight positive cases failed and six
  rejection cases passed. The checker rejected the still-present Recovery failure;
  raw Store fixture lines independently established Store RED.
  PBO SHA-256 `912865567EAD2409F2DCAA073610F595F15F9ADAA5608AD157140143248BBD90`;
  log SHA-256 `72A59DAE3B421D12A8881B9DF30473332E98D2EF29727A53C8E6A0D5D661212A`.
- Initial implementation boot compiled and produced Admission 11/11, Recovery 9/9,
  Store 25/25 expected results. Native JSON parsing of the intentionally truncated
  file emitted SCRIPT(E), so the overall verification correctly failed. Added a
  root-envelope check before native parsing; script-error rejection was retained.
  That failed-run log SHA-256 is
  `FA478E07F72066819FEBE3FEFFA16F2A8AE38BD4CE32D9E7132EA6A1537BA2BA`.
- Final native verification: PASS on dedicated server 1.29.163709, all modules loaded, Admission 11/11, Recovery 9/9 and Store 25/25, with no script errors. PBO SHA-256 `E9A0C60D706C316CA2F84FA42D0459A1E692DEF545F599B3E6C542C7F5814BA0`; captured log SHA-256 `3E1501C85BCB1F9F61575C3D1D8448FB446928687315EF38BE210C3422D6E68F`. The owned test process was stopped after evidence capture.
- Log-checker behavior: mixed known suites accepted; six negative synthetic logs
  rejected for cross-suite failure, malformed fixture, unknown fixture, duplicate
  other-suite case, script error and missing expected case.

Native corruption coverage uses only isolated synthetic fixture identities and
files: truncated newer generation, oversized newer file, generation gap, copied
contradictory generation, unsupported schema, invalid position, changed token,
unknown member and lost manifest participant. It checks exact roundtrip fields,
latest selection, previous-generation retention, error outputs and successful load
once the synthetic injected file is explicitly removed by the fixture. This is
not a live crash/restart matrix. Other complete-but-malformed JSON can still cause
the native parser to emit a script error; loading fails and the test gate fails.

Reproduction commands from the checkout:

```powershell
./tools/build.ps1
# Use the owned private-test runner with Suite Store and a fresh ignored profile.
./tools/check-fixtures.ps1 -LogPath <captured-script-log> -Suite Admission
./tools/check-fixtures.ps1 -LogPath <captured-script-log> -Suite Recovery
./tools/check-fixtures.ps1 -LogPath <captured-script-log> -Suite Store
```

The Store marker is `[SEV] Store fixtures complete`; the Recovery marker is
`[SEV] Recovery fixtures complete`. The combined package finishes with
`[SEV] GuardSpawn fixtures complete`. Actual run manifests/logs and synthetic profile
records remain in ignored local build profiles. Do not publish profiles or real
player records.

## Live marker and path-length findings

One real client exercised an isolated diagnostic build with marker writes enabled
and no inventory mutation. The first real-ID session failed at `session-create`:
the 44-character authenticated ID produced a 264-character session directory.
The manifest existed, but the session directory did not. Shortening the isolated
profile allowed the same marker probe to proceed.

The permanent fix uses manifest-v2 member slots. Native regression tests passed
under a 116-character profile root, longer than the original 110-character root.
All 149 fixtures passed: Admission 11, Recovery 9, Store 42, GuardSpawn 87. Store
cases cover 44-character case-distinct identities, wrong-slot contents, legacy
read/append without migration, and rejection of newly requested schema-1 manifests.
PBO SHA-256: `83B720895C75CC6CDE6EDF2A9E0B9675185A56CF0220BB74F52ACAA20ADA4E9A`.
Captured regression log SHA-256:
`B89380F3A92A8DBE50D1EC9055D41D14DBC8CFB366BCD66AF2D48C4D79352EFA`.

The live marker trial deliberately retained its original diagnostic artifact
while the path fix was developed. A normal logout and reconnect executed native
character load and returned `journal=true matching=true`. The server was then
terminated after that explicit save request and restarted with the same character
storage. Its diagnostic journal was copied unchanged to a fresh log profile.
With marker writes disabled in the new profile, the next character load again
returned `journal=true matching=true`. The same six item/clothing class names
plus the character entry were enumerated before and after; this is not a full
comparison of quantities, damage, or all serialized properties.

This establishes one basic marker roundtrip across logout and server restart.
It does not establish save acknowledgement, atomicity with inventory changes,
crash-at-transition recovery, legacy character compatibility, or inter-mod safety.
The diagnostic PBO SHA-256 is
`4DBFB59A3B7A7360F111328B19353F3E993539410722321EB09EF0817856CE59`.
Live logs and real player records remain local and untracked.
The final local restart-observation log SHA-256 is
`83C4F8C4837D98246339AC875A73C741DDA54D7371B29EDBDC026D8E8C6281EE`.

## Required live evidence still incomplete

| Boundary or behavior | Result |
| --- | --- |
| Terminate before write, partial generation, after readback | NOT RUN; only same-process file fault fixtures |
| Terminate before/after actual player save | NOT RUN |
| Terminate before/after actual character return receipt save | NOT RUN |
| Journal-to-character token mismatch after reconnect | NOT RUN; pure decision mismatch fixture only |
| Legacy character load and other-mod appended-field compatibility | NOT RUN |
| Connect/load/disconnect observation with real client | Basic logout/reload and server-restart marker roundtrip passed; earliest-interaction ordering NOT RUN |
| Earliest interaction versus recovery/quarantine | NOT RUN; no quarantine implemented |
| Distinguish real untouched/mutated/returned inventory across crashes | NOT RUN |
| Multi-client crash/restart matrix | NOT RUN; one real client available |

The journal and marker probe do not establish the required persistence guarantee.
Destructive admission and automatic destructive replay must remain disabled.
