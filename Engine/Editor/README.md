# Engine Editor

Editor-only modules live here.

- `OutputLog` owns the ImGui output log widget. It reads the runtime log bus and does not own gameplay/runtime logging state.

Do not put runtime systems here. Anything required by shipped samples or games belongs in `Engine/Runtime`.
