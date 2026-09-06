# Stationary guard replacement experiment

Status: planned experiment, not an implemented or validated freeze. The prior
server-only input-controller trial failed with uncontrolled movement. Its native
controller writes have been removed. Destructive admission remains disabled.

## Candidate and constraints

The candidate is an explicitly synchronized `HumanCommandScript` on the server
and owning client. Native interfaces expose zeroing animation translation before
physics, measuring and constraining position after physics, and finishing the
command through the normal move/fall/swim restoration path. The inspected vanilla
scripts contain no player implementation proving this approach works over the
network. Interface existence is not evidence of authority or safe restoration.

The server alone decides identity, token, command revision, and arm/release state.
A client acknowledgement is evidence of receipt, not permission to admit or
mutate inventory. A stale token, changed live character, unsafe command state, or
missing acknowledgement aborts the transition before mutation.

Only the active participant command may use native per-command physics callbacks.
This is an exception for constant work on that character, not a per-frame roster
scan or world search. The coordinator and validation stay on the bounded 250 ms
timer. Any future implementation must measure this callback cost for the maximum
roster and do no allocations or growing work in its physics callbacks.

Client input exclusions are an adjunct. They must have an established unused
baseline and an explicit owner during the isolated trial; never remove shared
groups because a different component's state cannot be observed. Neither these
exclusions nor position correction alone establish an authoritative freeze.

Do not borrow restraint, surrender, unconsciousness, vehicle, ladder, or swimming
states as neutral locks. Do not retry the withdrawn input-controller setters.

## First trial

1. Use a disposable character, no inventory mutation, and flat inland ground
   with at least 100 metres of clearance from water, cliffs and other hazards.
2. Before asking the player to begin, attach and verify an independent watchdog
   bound to the exact owned server process. It must survive a stalled game loop
   and must never target another or reused process ID.
3. Require normal on-foot move command, idle movement, a settled stance, no raised
   weapon/action/throw/inventory operation, and no vehicle/fall/swim/climb state.
   Observe at least two stable server samples. The owner releases movement input
   before the command handshake; record raw motion rather than assuming idle.
4. Arm the command for at most two seconds with no movement input. Record token,
   revision, command identity, activation/deactivation, pre-correction displacement,
   final position and maximum displacement on both peers. Corrected position alone
   must not hide an unstable command.
5. Abort on the first server sample exceeding 0.25 metres horizontally or
   0.50 metres vertically, a command/token mismatch, or any unsafe state. These
   are conservative experiment limits, not production tolerances. The independent
   watchdog also stops the server on a bound breach, missing heartbeat or hard
   deadline. Polling, log flushing and OS scheduling bound its reaction time;
   it cannot promise zero transient displacement.
6. On normal completion, finish only the exact command instance and observe the
   native fallback on both peers before removing owned input restrictions.
   C++ owns a submitted script-command instance; do not manually delete it.
7. Require three clean no-input runs before any two-second held-key trial. Then
   test jump, stance, attacks, inventory, damage, latency and reconnect separately.

The first trial must not silently grow into a long running/sprinting test.
Watcher completion is not a gameplay pass. Native command correction, client
animation, guard restoration, ordinary-player isolation and a third observer's
view all remain separate gates. No live execution follows merely from this plan.
