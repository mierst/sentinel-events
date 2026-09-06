# Diagnostic watcher evidence

## Native log transport, 2026-09-06

Two isolated DayZ 1.29.163709 servers ran a reviewed disposable mission that only
read a bounded trigger and emitted diagnostic log messages. No client connected;
no character, movement command, inventory, or gameplay state was involved. The
lead observed each watcher's unique ready artifact and verified its run, token,
and process identity before writing the matching trigger.

The watcher used commit `7da257f` and unchanged defaults: 100 ms polling, 3 seconds
armed, and 1 second without a heartbeat. The game package SHA256 was
`DDF028B4A1A1D5F577C90DEEE3714722368C6205395D1E60B20989604E01997A`.

| Case | Observed result |
| --- | --- |
| Normal log sequence | Armed, three synthetic zero-displacement samples, and finished were observed. Watcher finished about 1.45 seconds after launch and left the bound server running; lead subsequently stopped it. |
| Missing heartbeat | Armed was observed, then HeartbeatTimeout. The watcher successfully stopped its bound server. Total watcher elapsed time was about 2.01 seconds. |

These are transport and process-control results. The zero samples were constants,
not measurements. Neither case proves a stationary character, safe restoration,
or a hard real-time abort deadline. Native log flushing and operating-system
scheduling remain part of reaction latency.

Normal result JSON SHA256:
`D94064AF79C3BC740871132F4EEA6E2392CC95E9962C8EAAF40689E600D0FE51`.
Missing-heartbeat result JSON SHA256:
`F5CEA6B8EFC557CAF5C78A4D0F31E2CD53941CAC64B6C4B2C26D6A0D4B76C35F`.
Raw evidence and disposable missions remain local, outside the public package.

Both results exposed incorrect byte accounting: `AppendedBytes` remained zero
because the read function updated a function-local counter. This also weakened
the cumulative byte limit into a per-read limit. The transport results above do
not establish that byte-budget protection; the separate regression and fix below
address that defect.

## Cumulative-byte regression

The normal-finish check was extended to compare the result counter with the exact
log-length increase. A separate test appended four individually small batches
over different reads and required the cumulative limit to stop the owned helper.
Both failed before the fix: the counter reported zero, and the over-limit stream
finished successfully. Explicit script scope for the counter fixes both cases
in commit `14b3d40`; independent review approved the fix and all 12 process-double
scenarios pass. This scope-only correction has not been
rerun against a native server. The native transport evidence above belongs to the
earlier revision and is not being relabelled as a test of the corrected counter.
