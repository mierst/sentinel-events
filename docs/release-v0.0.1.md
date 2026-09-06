# v0.0.1 release candidate gates

Status: implementation in progress. No Workshop upload has been performed.
The release target is the accepted single-active-event battle royale described
in [the v1 design](v1/design.md), not the admission-only rehearsal.

| Gate | Required evidence | Current state |
| --- | --- | --- |
| Standalone package | Dedicated-server and client compilation, explicit version and dependencies | Diagnostic server build passes 219 fixtures; ready menu opened in a client; corrected UI appearance and release package pending |
| Administration | Owner grants, per-request authority, approved templates, scheduling, registration, audit | Authenticated readiness rehearsal implemented; full roles, scheduling, and audit pending |
| Admission | Consent, eligibility, shared deadline, preflight before stripping, safe spawn, countdown | Pure policy passes; mutation remains disabled |
| Recovery | Persistent origin and character receipts, crash boundaries, offline and replacement returns | Basic one-client marker roundtrip passed across logout/restart; destructive recovery unproven |
| Isolation | Movement/action/transfer guards, safe outsider relocation, ordinary-player control tests | Server controller freeze FAILED and withdrawn; replacement and transfer/multiplayer gates unresolved |
| Combat | Lootable contestants, tracked item ownership, boundary/disconnect forfeits, tie and time-limit policy | Pending |
| Settlement | One durable winner, cancellation cutoff, retained carried inventory, safe return | Pending |
| Claims | Partial fit, no ground drops, expiry, reconnect/restart, interrupted delivery and replay | Pending |
| Spectating | Optional platform/podium, damage and transfer isolation, containment, safe exits | Pending |
| Performance | Bounded work measured at 2 and 8 participants with ordinary-player control | Pending |
| Distribution | Icon and textures, attribution/license, signed PBO, clean package, install instructions | Icon generated; remaining work pending |

Record each runtime result with its build identifier, game version, exact test
case and fresh log. Distinguish pass, fail, inconclusive, and not run. Compilation
and pure fixtures cannot substitute for client/server or persistence evidence.
The detailed admission matrix is required before enabling inventory destruction.

Before packaging the candidate:

1. Complete and review the accepted gameplay scope and the gates above.
2. Make diagnostic fixtures and fault injection opt-in; exclude test state and
   credentials from the distribution. Verify that a normal install is disabled
   until the operator supplies valid configuration and administrative identities.
3. Set the same `0.0.1` version in source metadata, distribution metadata, and
   release notes. Build from the reviewed commit and record the output hashes.
4. Convert the reviewed icon to the required texture format and inspect it in
   the actual client. Check the small event-menu attribution and Workshop text.
5. Sign the final PBO with an owner-controlled private key outside the repository.
   Include only its public key and signature in the installable package. Test a
   fresh client/server installation with signature verification enabled.
6. Inspect the staged archive: mod metadata, PBO, signature, public key, license,
   and installation instructions only. No profiles, player records, local notes,
   private keys, game assets, or unrelated integrations.
7. Prepare the Workshop description, change notes, preview, and upload manifest
   for owner review. The candidate remains unpublished until the owner requests
   publication.

An unresolved gameplay or recovery gate is a release blocker. Record the failing
case and next experiment; do not remove a safety check to produce an uploadable
artifact.
