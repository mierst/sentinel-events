# Event administration permissions

This is the implementation contract for the accepted standalone permission
model. It describes intended behavior; runtime verification is tracked separately.

The server owner grants roles to stable Steam64 identities in server-side
configuration. Player display names and client-supplied identities never grant
authority. Empty or invalid configuration grants nobody event administration.

| Role | Granted actions |
| --- | --- |
| Organizer | Schedule approved templates, manage registration for their own runs, inspect their own runs, cancel their own runs |
| Designer | Organizer actions plus create, edit, approve, import, and export templates, kits, and rewards |
| Event admin | Designer actions plus inspect, manage, and cancel any run |

Only the server owner edits role assignments through server configuration.
The event UI cannot grant roles, promote its caller, or edit the permission file.
An event admin may request a server-side reload; the service validates the entire
replacement before applying it. An invalid reload preserves the last valid
configuration and reports the error. A cold start with invalid permissions remains
disabled. Successful reloads immediately affect subsequent requests, including
requests from already-open menus.

Scheduling captures an immutable approved template revision and the organizer's
authenticated identity. Editing a template cannot change an existing run. A
template approval is invalidated by edits; an organizer cannot substitute kit,
reward, arena, or timing-policy fields in a scheduling request. Requested schedule
times must satisfy the captured template's validated limits.

The server checks role, action, event ownership, run phase, and request revision
on every administrative request. It derives the caller from the RPC sender and
rejects missing identities, unknown actions, stale revisions, and excessive
requests before expensive work. Hiding a menu control is only a presentation
choice. Role revocation does not silently cancel a scheduled event; another
authorized administrator can manage it.

Keep a local administrative audit recording server timestamp, authenticated
actor, action, target and revision, and outcome. Record permission reloads and
rejections without copying raw RPC payloads, secrets, or whole configurations.
Audit records remain in the server profile and are never packaged with the mod.

Permission fixtures cover every role/action pair, another organizer's run,
missing and spoofed identities, stale menu requests after revocation, invalid
reloads, edited-but-unapproved templates, and duplicate scheduling requests.
Client/server trials additionally verify that direct RPC requests cannot bypass
the checks shown by the UI.
