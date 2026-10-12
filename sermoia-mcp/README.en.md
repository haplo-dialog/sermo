<img src="icon/sermoia-mark.png" alt="sermoia" width="88" align="right">

# sermoia-mcp — an MCP server for writing sermo with an AI

sermoia-mcp gives an AI assistant (Claude, or any **MCP** client) the tools to write a
sermo dialog, have the real parser **check** it, **see** it as a picture and **fix**
it, before showing it to the user.

It is sermo's second MCP server, next to [sermoman-mcp](../sermoman-mcp/): sermoman
**explains** the language (the manual), sermoia **puts it into practice**. Written in C
(GLib, GIO, json-glib). It speaks JSON-RPC 2.0 on its standard input and output, one
message per line. GPL-2.0-or-later.

## The recommended loop

1. `sermo_schema` once: the list of tags and actions.
2. `sermo_examples` with what the dialog must do ("contact form"): already validated
   dialogs to imitate.
3. Write the XML.
4. `sermo_check`: fix until "OK".
5. `sermo_render`: look at the layout.
6. `sermo_run`, **only if the user wants to try it**: it is the only tool that runs the
   dialog's commands.
7. `sermo_vote`: say whether the example you followed helped.

The server sends this summary to the client as soon as it starts (the `instructions`
field of `initialize`).

## The six tools

| Tool | Role | Does it run the XML's commands? |
|---|---|---|
| `sermo_schema` | describes the language | no |
| `sermo_check` | has an XML parsed | no |
| `sermo_render` | draws the dialog as a PNG | **no, they are blocked** |
| `sermo_run` | shows the dialog to the user, returns the values | **yes** |
| `sermo_examples` | serves validated examples | no |
| `sermo_vote` | rates an example | no (appends one line to the register) |

Each tool carries MCP hints (`readOnlyHint`, `destructiveHint`, `openWorldHint`), so a
client can ask for confirmation before `sermo_run`, the only one marked "destructive".

### `sermo_schema`

No argument. Returns a JSON object:
- the widgets, grouped by family; the test checks this list against the tags the core
  parser recognises;
- the content tags, the common attributes, the actions and the signals;
- a minimal example and the shape of the output.

The attributes specific to each widget are in the manual (sermoman-mcp).

### `sermo_check`

| Argument | Type | |
|---|---|---|
| `xml` | string, **required** | the whole dialog, `<window>` root |
| `ir` | boolean | `true`: adds the internal representation read by the parser |

Runs `sermo --print-ir`, which parses without showing or running anything. Returns:
- `OK — le XML s'analyse (…)` when the parser accepts the XML;
- otherwise `ERREUR — l'analyseur refuse ce XML.`, followed by the parser's message and
  a hint, with `isError`.

Example message: `sermocore: Error in line 1, near token '<bouton/>': Unknown tag.`

Messages from the graphics libraries that say nothing about the XML ("Gtk-Message"…)
are removed. Tool messages are in French, like the rest of the server.

### `sermo_render`

| Argument | Type | |
|---|---|---|
| `xml` | string, **required** | the whole dialog |

Draws the dialog as a PNG without showing it, and returns the picture followed by a
caption (port used, size). **Every command in the XML is blocked** while rendering
(`<input>`, `<action>`, including through `sh -c`): asking for a preview never runs
anything. The downside: a widget filled by a command looks empty in the picture.

Depending on the chosen port (`SERMO_RENDER_BIN`):

| Port | How |
|---|---|
| qt6, sdl3, efl1 | the toolkit's offscreen platform, no display needed |
| gtk3, gtk4, fltk1, lvgl | `--render-png` under `xvfb-run` (`xvfb` package) |
| ncurses, html | no window: a **structural** text preview (`--print-ir`) |

If no picture comes out (failed render, timeout, `xvfb-run` missing), the tool returns
the structural preview and says why. A picture smaller than 16 × 16 pixels is rejected:
qt6 once rendered only the drop-down list, at 2 × 2.

### `sermo_run`

| Argument | Type | |
|---|---|---|
| `xml` | string, **required** | the whole dialog |
| `timeout` | integer | seconds left to the user, 5 to 3600 (default 300) |

Opens the dialog in a real window, waits for the user to close it, and returns what
sermo prints: one `NAME="value"` line per named widget, then `EXIT="…"`. The XML's
commands **run**, as they would in a script: only call it with the user's consent, on
a checked XML.

A display is needed (`DISPLAY` or `WAYLAND_DISPLAY`). Without one, the tool returns a
clear error instead of "no value". After the timeout, the dialog is closed by force,
with no values.

### `sermo_examples`

| Argument | Type | |
|---|---|---|
| `task` | string | what the dialog must do: a word or a sentence |
| `limit` | integer | number of examples returned, 1 to 10 (default 4) |

Without `task`: the catalogue map (topics, number of examples, vote).

With `task`, the sentence is split into words; stop words ("the", "for", "dialog"…, and
their French equivalents) are dropped. Each topic scores:
- 3 points per word found in its name;
- 1 point per word found in its files.

Topics are ranked by score, then by vote, then by name, and each example is returned
whole. If nothing matches, the tool returns the list of topics.

Topic names are in French or are widget names (`formulaire` = form, `liste-selection`
= selection list…).

### `sermo_vote`

| Argument | Type | |
|---|---|---|
| `example` | string, **required** | `topic` or `topic/file.xml`, as `sermo_examples` gave it |
| `helpful` | boolean, **required** | `true`: helped (+1); `false`: misled (−1) |
| `note` | string | why, in one sentence |

Appends one line to the register (`topic<TAB>±1<TAB>note`): nothing is ever deleted. A
topic missing from the catalogue is refused.

## Security

- **What runs commands**: `sermo_run`, and only it. A sermo dialog is meant to run
  commands: `sermo_run` runs them, as the script would. To limit what it may run, set
  `SERMO_ALLOWED_CMDS` in the server's environment (see `SECURITY.en.md` at the root):
  the core then refuses any command outside the list, and the `/bin/sh -c` fallback.
- **What does not**: `sermo_check` (parsing only) and `sermo_render`. Rendering sets
  `SERMO_ALLOWED_CMDS` to a command that does not exist, which forbids everything. The
  test checks it: an `<input>` that creates a witness file does not create it.
- **The MCP channel is protected**: a launched process reads `/dev/null`, never the
  channel, and its output is captured. A port that prints its `VAR="…"` even when
  rendering (html) therefore does not break the protocol.
- **Temporary files**: the XML and the picture go through a private directory (0700),
  removed after each call.
- **Timeouts**: when time is up, the whole process group is killed (`xvfb-run`, Xvfb and
  the port), and the server still answers afterwards.
- **Oversized messages**: a message over 8 MiB is refused.

## Environment variables

| Variable | Default | |
|---|---|---|
| `SERMO_BIN` | `sermo` | binary for `check`, `run` and the text preview |
| `SERMO_RENDER_BIN` | `SERMO_BIN` | binary for `render` (a windowed port to get a picture) |
| `SERMO_EXAMPLES_DIR` | the package's, otherwise `examples` | example catalogue |
| `SERMO_VOTE_FILE` | `$XDG_STATE_HOME/sermoia/votes.tsv` | vote register |
| `SERMOIA_CHECK_DELAI` | 20 s (≤ 120) | parsing timeout |
| `SERMOIA_RENDER_DELAI` | 30 s (≤ 300) | timeout of each render attempt |
| `SERMO_ALLOWED_CMDS` | — | commands `sermo_run` may launch (see Security) |

## Install

The packages are attached to each release; check the signed checksums
(`SHA256SUMS.asc`) first, as the [README](../README.en.md#install) explains.

```sh
sudo apt install ./sermoia-mcp_2.8.2-3_amd64.deb ./sermo-backend-qt6_2.8.2-3_amd64.deb
```

The package installs `/usr/bin/sermoia-mcp` and its catalogue in
`/usr/share/sermoia-mcp/examples` (which the server finds by itself), job scripts
and toolboxes included. It needs at least one sermo port; `sermo-backend-qt6` is recommended: it
renders pictures without a display or `xvfb`.

## Build, run, check

```sh
cmake -S sermoia-mcp -B sermoia-mcp/_build && cmake --build sermoia-mcp/_build
SERMO_BIN=/usr/bin/gtk3sermo SERMO_RENDER_BIN=/usr/bin/qt6sermo \
SERMO_EXAMPLES_DIR=sermoia-mcp/examples sermoia-mcp/_build/sermoia-mcp
bash sermoia-mcp/tests/verifie.sh
```

Dependencies: `libglib2.0-dev` (GLib and GIO), `libjson-glib-dev`; `xvfb` for pictures
with gtk3, gtk4, fltk1 or lvgl. The version announced to clients is the one in the
repository's `VERSION` file.

`tests/verifie.sh` runs five checks on the real code and binaries:

1. **Schema**: it names exactly the core parser's widget tags.
2. **Catalogue**: every example parses.
3. **Protocol and tools**:
   - initialisation and version negotiation;
   - six annotated tools;
   - right and wrong check, with no GTK noise;
   - examples searched by word and by sentence;
   - vote, with an off-catalogue topic refused and the note sanitised;
   - `ping` and JSON-RPC error codes;
   - a render that leaves nothing on the channel and does not run a witness `<input>`.
4. **Picture**: a real PNG with a windowed port, still without running anything.
5. **Timeout**: a dialog that is never closed is cut at 5 s, with no process left.

Four mutations of the code (commands not blocked when rendering, small pictures
rejected, timeout ignored, `ping` refused) each make the test fail.

## Declaring the server in an MCP client

Claude Code:

```sh
claude mcp add sermoia --env SERMO_BIN=/usr/bin/gtk3sermo --env SERMO_RENDER_BIN=/usr/bin/qt6sermo -- /usr/bin/sermoia-mcp
```

Other clients (JSON configuration file):

```json
{
  "mcpServers": {
    "sermoia": {
      "command": "/usr/bin/sermoia-mcp",
      "env": {
        "SERMO_BIN": "/usr/bin/gtk3sermo",
        "SERMO_RENDER_BIN": "/usr/bin/qt6sermo"
      }
    }
  }
}
```

With `qt6sermo` as `SERMO_RENDER_BIN`, rendering needs neither a display nor `xvfb`.

## The protocol, by hand

```sh
printf '%s\n' \
  '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18"}}' \
  '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"sermo_check","arguments":{"xml":"<window><vbox><bouton/></vbox></window>"}}}' \
  | SERMO_BIN=/usr/bin/gtk3sermo sermoia-mcp/_build/sermoia-mcp
```

The second answer carries `isError: true` and the text
`ERREUR — l'analyseur refuse ce XML. sermocore: Error in line 1, near token '<bouton/>': Unknown tag. …`.

Protocol points:
- accepted versions: 2025-06-18, 2025-03-26 and 2024-11-05; the server echoes the
  client's version when it knows it;
- `ping` is supported;
- notifications are read without an answer;
- error codes: -32700 (unreadable JSON), -32600 (invalid request), -32601 (unknown
  method), -32602 (unknown tool);
- JSON-RPC batches are not supported.

## Example catalogue (`examples/`)

Each subdirectory is a **topic**, each `.xml` a whole dialog that the test has parsed.
Three families:

- **dialog patterns**, useful for generation: `formulaire`, `saisie-simple`,
  `confirmation`, `information`, `mot-de-passe`, `cases-a-cocher`, `choix-radio`,
  `zone-texte`, `liste-selection`, `liste-deroulante`, `onglets`, `tableau`, `arbre`;
- **widgets**, one minimal dialog per widget: `button`, `checkbox`, `entry`,
  `combobox`, `spinbutton`, `calendar`, `list`, `table`, `tree`, `notebook`, `menu`,
  `timer`…;
- **job scripts and toolboxes**: 16 topics (`metier-…` for the 12 jobs, `boite-…`
  for the 4 toolboxes), 187 dialogs taken from the scripts of `examples/metiers/`
  and `examples/boites-a-outils/` by `outils/catalogue-metiers.sh`. Each XML names
  its source script in a comment.

Topic and file names are limited to letters, digits, `-`, `_` and `.`. Point
`SERMO_EXAMPLES_DIR` elsewhere to serve another catalogue.

## Known limits

- **efl1**: without `width-request`, an empty entry field comes out with no width in
  the picture (a port defect, measured on 2026-10-11).
- **html and ncurses** have no picture: the preview is the `--print-ir` text.
- **`sermo_run` with ncurses**: the server has no terminal to give it, so the dialog
  ends at once (`EXIT="abort"`).
- **No parallel calls**: requests are handled one at a time, and a `sermo_run` waits for
  the dialog to be closed.
