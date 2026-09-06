# Contributor guidance for agents

Read [CLAUDE.md](CLAUDE.md) beside this file for shared project facts and
[CONTRIBUTING.md](CONTRIBUTING.md) for engineering requirements. Resolve all
references in this checkout, not a sibling checkout.

Retain your own identity, native tools, and instruction precedence. Guidance
written for another harness does not import its runtime or orchestration rules.

Bounded independent work may use native subagents. The lead owns requirements,
integration, and verification; give workers distinct scopes and avoid conflicting
edits. Do not treat proposed defaults or open questions as approved requirements.

This is a public repository. Follow CONTRIBUTING.md's public/private boundary.
Never publish private project references or behavior. If local cross-project
context is necessary, consult `.git/local-context/README.md` when present and
keep that material local; it is not part of the public project specification.
