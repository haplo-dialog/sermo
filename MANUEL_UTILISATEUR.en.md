# User Manual — sermo

[Français](MANUEL_UTILISATEUR.md)

**Version:** 2.7.7
**Licence:** GPL-2.0-or-later | **Distributor:** haplo-dialog (devel@haplo-dialog.fr)

---

## Table of contents

1. [Introduction](#1-introduction)
2. [Installation](#2-installation)
3. [First steps](#3-first-steps)
4. [Basic syntax](#4-basic-syntax)
5. [Widget reference](#5-widget-reference)
6. [Actions and signals](#6-actions-and-signals)
7. [Variables and input/output](#7-variables-and-inputoutput)
8. [Practical examples](#8-practical-examples)
9. [Integrating into a shell script](#9-integrating-into-a-shell-script)
10. [FAQ and troubleshooting](#10-faq-and-troubleshooting)

---

## 1. Introduction

`sermo` is a command-line utility that builds **graphical interfaces** from any
shell, Python, Perl or other interpreted script — without writing a single line
of graphical code.

The principle is simple: you describe your interface in **XML**, and sermo
displays it. When the dialog ends **through a button**, the values entered are
returned on standard output as shell variables.

**This manual describes the XML language**, which is **identical across sermo's
seven backends** (gtk3, gtk4, qt6, fltk1, efl1, sdl3, ncurses): the same script
renders an equivalent interface everywhere. In this document `sermo` means the
backend selected by default on your system (`update-alternatives`), but you can
also call a specific backend (`gtk3sermo`, `qt6sermo`…).

**Minimal example:**

```bash
export DIALOG='
<window title="Welcome">
  <vbox>
    <text><label>Enter your name:</label></text>
    <entry><variable>NAME</variable></entry>
    <button><label>OK</label><action>EXIT:ok</action></button>
  </vbox>
</window>'

sermo --program=DIALOG
# Output: NAME="John"  EXIT="ok"
```

### 1.1 Compatibility

sermo is **compatible** with historical gtkdialog scripts. The virtual
`gtkdialog` package points at the GTK 3 backend, so scripts calling `gtkdialog`
keep working. Visible differences:

- The look follows the **current system theme** (light/dark).
- Colours are given as `rgba(r,g,b,a)` or `#RRGGBB`.
- The embedded terminal requires VTE and exists only on gtk3sermo and gtk4sermo.

---

## 2. Installation

### 2.1 From packages

sermo ships **separate** packages — a common core plus one backend per toolkit:

```bash
apt install sermo-backend-gtk3     # GTK 3
apt install sermo-backend-fltk1    # FLTK, without pulling in GTK or Qt
apt install sermo-backend-qt6      # Qt 6
```

The `sermo` alias (`/usr/bin/sermo`) is managed by `update-alternatives`: every
installed backend registers there and you choose the default.

### 2.2 From source

See [COMPILE.en.md](COMPILE.en.md). In short, you first build the `libsermocore`
core, then the backend of your choice against that core.

### 2.3 Checking the installation

```bash
echo '<window><vbox>
  <text><label>sermo works!</label></text>
  <button><label>Close</label><action>EXIT:ok</action></button>
</vbox></window>' | sermo --stdin
```

A window should appear. If it opens, the installation succeeded.

---
## 3. First steps

### 3.1 Ways to use it

sermo accepts its XML in three ways:

**From stdin:**
```bash
echo '<window>...</window>' | sermo --stdin
```

**From an environment variable:**
```bash
export MY_DIALOG='<window>...</window>'
sermo --program=MY_DIALOG
```

**From a file:**
```bash
sermo --file=my_interface.xml
```

### 3.2 Getting the values back

When the dialog ends through an `EXIT:` action — which is what a button does —
sermo prints on stdout the values of every named widget, then the `EXIT` line. To
use them in your script:

```bash
export DIALOG='
<window title="Form">
  <vbox>
    <entry><variable>FIRSTNAME</variable></entry>
    <entry><variable>LASTNAME</variable></entry>
    <button><label>Confirm</label><action>EXIT:confirm</action></button>
    <button><label>Cancel</label><action>EXIT:cancel</action></button>
  </vbox>
</window>'

# --do: the values arrive as environment variables, never going back through
# the shell. This is the recommended route (see §8.1).
sermo --program=DIALOG --do='
    if [ "$EXIT" = "confirm" ]; then
        echo "Hello $FIRSTNAME $LASTNAME!"
    fi'
```

⚠️ **Closing the window with the cross returns no value at all.** If the user
closes through the window manager (the cross, `Alt+F4`), the whole output is
`EXIT="abort"`: no `FIRSTNAME=…` line. Measured on 2026-09-19 against the 2.7.1
package binary. Likewise, the `closewindow:` action on the last window returns
only `EXIT="closewindow"`. Your script must therefore treat both values as an
abandon, and rely on the variables only when `EXIT` carries the value of one of
your buttons.

### 3.3 Off-screen PNG rendering (`--render-png`)

To produce an image of the interface **without opening a window** — a
deterministic capture for documentation, a thumbnail or a visual test:

```bash
sermo --render-png output.png --file=my_interface.xml
```

Off-screen rendering is available on **all six graphical backends** (`gtk3`,
`gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3`) — `gtk3`/`gtk4`/`fltk1` through a virtual
display (xvfb) where the backend writes the PNG itself, `qt6`/`efl1`/`sdl3`
through their offscreen platform. `ncurses` stays a **terminal** display (no
image; text preview through `--print-ir`).

The size asked for through `default-width`/`default-height` is honoured on all
six. Up to 2.7.1, `qt6` ignored it and sometimes produced a 2×2 pixel image when
the window held a drop-down list — `tests/garde_taille_fenetre.sh` now checks
this on every graphical port.

---

## 4. Basic syntax

### 4.1 Structure of a document

```xml
<window title="Window title" resizable="true" width="400" height="300">
  <vbox>
    <text><label>Your widgets here</label></text>
  </vbox>
</window>
```

Every sermo document starts with `<window>`. Widgets are nested inside containers
(`<vbox>`, `<hbox>`, `<frame>`, `<notebook>`).

### 4.2 Attributes common to all widgets

| Attribute | Values | Description |
|----------|---------|-------------|
| `sensitive` | `true` / `false` | Enable/disable the widget |
| `visible` | `true` / `false` | Show/hide the widget |
| `tooltip-text` | text | Tooltip on hover |
| `width-request` | number | Minimum width in pixels |
| `height-request` | number | Minimum height in pixels |

### 4.3 Common content tags

| Tag | Description |
|--------|-------------|
| `<variable>NAME</variable>` | Name of the variable exported on output (see §3.2) |
| `<label>text</label>` | Label shown in the widget |
| `<default>value</default>` | Initial value of the widget |
| `<input>command</input>` | Shell command whose output feeds the widget |
| `<action>ACTION:arg</action>` | Action triggered by an interaction |
| `<sensitive>false</sensitive>` | Disable the widget at startup |
| `<width>N</width>` and `<height>N</height>` | Sub-element form of `width-request` / `height-request` |

**Every tag is written opened then closed.** The self-closing form is not
recognised by the parser: `<separator></separator>` and not `<separator/>`,
`<button ok></button>` and not `<okbutton/>`. An empty container is refused too:
a `<vbox>` must contain at least one widget.

---

## 5. Widget reference

### 5.1 Containers

#### `<window>` — Main window

```xml
<window title="My Application" resizable="true" width="500" height="400">
  <vbox>
    <text><label>Window content</label></text>
  </vbox>
</window>
```

Specific attributes: `title`, `resizable`, `width`, `height`, `decorated`,
`icon-name`.

#### `<vbox>` and `<hbox>` — Layout boxes

```xml
<vbox space-expand="true" space-fill="true">
  <hbox homogeneous="false" spacing="5">
    <text><label>Left</label></text>
    <text><label>Right</label></text>
  </hbox>
</vbox>
```

`<vbox>` stacks vertically, `<hbox>` horizontally. Attributes: `homogeneous`,
`spacing`, `space-expand`, `space-fill`.

#### `<frame>` — Titled frame

```xml
<frame label="Options" label-xalign="0.0">
  <vbox><text><label>Frame content</label></text></vbox>
</frame>
```

#### `<notebook>` — Tabs

```xml
<notebook tab-labels="Tab 1|Tab 2">
  <vbox><text><label>Content of tab 1</label></text></vbox>
  <vbox><text><label>Content of tab 2</label></text></vbox>
</notebook>
```

#### `<expander>` — Collapsible section

```xml
<expander expanded="true">
  <vbox><text><label>Collapsed content</label></text></vbox>
  <label>Details</label>
  <variable>OPEN</variable>
</expander>
```

A section the user opens and collapses. It **exports its state**: `true` when
open, `false` otherwise.

- `expanded="true"` (or `yes`, or `1`) opens it at startup. **Without that
  attribute the section is collapsed** — that is the reference behaviour.
- The title can be written either way: as a tag attribute `label="Details"`, or
  as an element `<label>Details</label>`. Both work.
  ⚠️ **The `<label>` element goes AFTER the content**, with the other attributes:
  the grammar is `<expander>` content, then attributes. Placed before it, the
  parser refuses it — `syntax error near '<label>'`.
  ⚠️ The short form `<expander Title>` does not exist here.

#### Scrollable area — the `scrollable` attribute

There is **no** `<scrolledwindow>` tag. Scrolling is an **attribute** set on the
widget itself:

```xml
<tree scrollable="true">
  <variable>CHOICE</variable>
  <label>Name|Size</label>
  <input>echo -e "a|1\nb|2"</input>
</tree>
```

The attribute is recognised by the core's automaton and wraps the widget in a
scrollable area when it is created.

---

### 5.2 Input widgets

#### `<entry>` — Single-line text field

```xml
<entry>
  <variable>TEXT</variable>
  <default>initial value</default>
  <action signal="activate">EXIT:ok</action>
</entry>
```

#### `<edit>` — Multi-line text area

```xml
<edit width-request="400" height-request="200">
  <variable>CONTENT</variable>
  <default>Line 1
Line 2</default>
</edit>
```

#### `<spinbutton>` — Numeric selector

```xml
<spinbutton range-min="0" range-max="100" range-step="1" digits="0">
  <variable>VALUE</variable>
  <default>50</default>
</spinbutton>
```

#### `<hscale>` / `<vscale>` — Slider

```xml
<hscale range-min="0" range-max="255" range-step="1" draw-value="true">
  <variable>BRIGHTNESS</variable>
  <default>128</default>
</hscale>
```

---

### 5.3 Selection widgets

#### `<checkbox>` — Checkbox

```xml
<checkbox>
  <label>Enable notifications</label>
  <variable>NOTIF</variable>
  <default>true</default>
  <action>REFRESH:OTHER_WIDGET</action>
</checkbox>
```

The variable is `true` or `false`.

#### `<radiobutton>` — Radio button (exclusive choice)

```xml
<vbox>
  <radiobutton><label>Option A</label><variable>CHOICE_A</variable></radiobutton>
  <radiobutton><label>Option B</label><variable>CHOICE_B</variable></radiobutton>
</vbox>
```

#### `<comboboxtext>` — Simple drop-down list

```xml
<comboboxtext>
  <variable>COLOUR</variable>
  <item>Red</item>
  <item>Green</item>
  <item>Blue</item>
  <default>Green</default>
</comboboxtext>
```

#### `<list>` — List with selection

```xml
<list>
  <variable>SELECTION</variable>
  <item>Item 1</item>
  <item>Item 2</item>
</list>
```

#### `<tree>` — Multi-column tree/table

```xml
<tree selection-mode="single" column-header-active="true">
  <variable>ROW</variable>
  <label>Name|Size|Date</label>
  <input>ls -lh --time-style=short | awk 'NR>1{print $9"|"$5"|"$6" "$7}'</input>
</tree>
```

---

### 5.4 Buttons and actions

#### `<button>` — Generic button

```xml
<button>
  <label>Click here</label>
  <action>EXIT:clicked</action>
</button>
```

With an icon:
```xml
<button>
  <input file stock="gtk-open"></input>
  <label>Open</label>
  <action>FILESELECT:FILE</action>
</button>
```

#### Standard dialog buttons

```xml
<hbox>
  <button ok></button>
  <button cancel></button>
</hbox>
```

These buttons carry the standard labels and shortcuts and produce `EXIT="OK"` or
`EXIT="Cancel"`.

Five forms exist: `<button ok>`, `<button cancel>`, `<button help>`,
`<button yes>` and `<button no>`. They are written **exactly like that** — a
single space, and a closing `</button>` tag: the parser recognises them as a
whole (`gtkdialog_lexer.l`), there is no `<okbutton/>` tag.

#### `<togglebutton>` — Toggle button

```xml
<togglebutton>
  <label>Enable</label>
  <variable>STATE</variable>
  <default>false</default>
  <action>REFRESH:OTHER</action>
</togglebutton>
```

---

### 5.5 Display

#### `<text>` — Text label

```xml
<text use-markup="true">
  <label>&lt;b&gt;Bold text&lt;/b&gt; and &lt;i&gt;italic&lt;/i&gt;</label>
</text>
```

#### `<pixmap>` — Image

```xml
<pixmap width-request="64" height-request="64">
  <input file>/path/to/image.png</input>
</pixmap>
```

#### Common rule for numeric values

`<hscale>`, `<vscale>`, `<spinbutton>` and `<levelbar>` export their value
according to the **`digits=`** attribute: `digits="0"` (the default) gives an
integer, `digits="2"` gives two decimals.

The number is **always written with a dot**, never a comma, whatever the
machine's locale. A script comparing against `"2.50"` gets the same thing
everywhere.

`<progressbar>` **exports nothing**: it is a display, not an input.

#### `<progressbar>` — Progress bar

```xml
<progressbar>
  <variable>PROGRESS</variable>
  <input>echo 75</input>
</progressbar>
```

The expected value is a **percentage, between `0` and `100`** — not a fraction.
This manual long claimed "between `0.0` and `1.0`", and its example wrote
`echo 0.75`: measured on 2026-09-20 against sdl3sermo, `0.75` gives a bar at
**0 %**, `75` fills it three quarters of the way.

⚠️ **Write an integer.** gtk3 reads the value with `strtol`: `0.75` is `0` there.
The neutral ports divide by 100: `0.75` gives 0.75 % — empty to the eye. A
decimal does not "sort of work" anywhere.

`<progressbar>` **exports nothing**: it is a display, not an input. For a level
gauge, see `<levelbar>`, which has its own `range-min` / `range-max`.

#### `<statusbar>` — Status bar

```xml
<statusbar>
  <variable>STATUS</variable>
  <default>Ready</default>
</statusbar>
```

---

### 5.6 Special widgets

#### `<colorbutton>` — Colour picker

```xml
<colorbutton>
  <variable>COLOUR_HEX</variable>
  <default>#ff6600</default>
</colorbutton>
```

#### `<fontbutton>` — Font picker

```xml
<fontbutton>
  <variable>FONT</variable>
  <default>Sans 12</default>
</fontbutton>
```

#### `<terminal>` — Embedded terminal (gtk3sermo and gtk4sermo, requires VTE)

```xml
<terminal width-request="600" height-request="300">
  <variable>TERMINAL</variable>
  <input>echo "ls -la"</input>
</terminal>
```

The terminal starts a shell (`/bin/sh`, or the `argv0`, `argv1`… attributes). The
**output** of the `<input>` command is typed into that shell, as if from the
keyboard: here, `ls -la` runs in the terminal. The variable returns the shell's
PID.

#### `<timer>` — Timer

```xml
<timer milliseconds="true" interval="1000" visible="false">
  <variable>CLOCK</variable>
  <action>REFRESH:DISPLAY</action>
</timer>
```

Fires an action at a regular interval: `interval` counts whole seconds, or
milliseconds when `milliseconds="true"` (a boolean, not a duration).

---

### 5.7 Menus

```xml
<menubar>
  <menu label="File">
    <menuitem>
      <label>Open</label>
      <action>FILESELECT:FILE</action>
    </menuitem>
    <menuitemseparator></menuitemseparator>
    <menuitem>
      <label>Quit</label>
      <action>EXIT:quit</action>
    </menuitem>
  </menu>
</menubar>
```

### 5.8 Additional widgets

Available on every backend (native on GTK, reimplemented elsewhere):

#### switch — On/off switch

```xml
<switch><variable>MY_SWITCH</variable><default>true</default></switch>
```

Variable: `true` when on, `false` otherwise.

#### filechooser — File/folder picker

```xml
<filechooser>
  <label>Choose a file</label>
  <variable>FILE</variable>
  <default>/home/user</default>
</filechooser>
```

For a folder: `<filechooser action="select-folder">`.

The `<chooser>` tag, on the other hand, embeds the picker INSIDE the window (GTK
3 and GTK 4). On the five other ports it becomes a `<filechooser>`: a button
opening the toolkit's dialog.
Variable: absolute path of the selected file/folder.

Depending on the port: gtk3/gtk4 and qt6 open their toolkit's picker, `sdl3` the
**system** one (XDG portal), `efl1` `elm_fileselector`, `ncurses` a browser
**inside the terminal** (arrow keys, `..`, `h` for hidden files).

#### calendar — Date picker

```xml
<calendar><variable>DATE</variable><default>2026-05-21</default></calendar>
```

Variable: date in ISO 8601 `YYYY-MM-DD` format.

#### linkbutton — Hyperlink button

```xml
<linkbutton>
  <label>Visit haplo-dialog</label>
  <default>https://haplo-dialog.fr</default>
  <variable>LINK</variable>
</linkbutton>
```

Variable: the URI (`<default>`), not the displayed label. Clicking opens the
address in the default application.

#### searchentry — Search field

```xml
<searchentry>
  <label>Search...</label>
  <variable>TERM</variable>
  <action>grep "$TERM" /var/log/syslog | head -20</action>
</searchentry>
```

#### infobar — Notification bar

```xml
<infobar>
  <label>Operation succeeded.</label>
  <default>info</default>
  <variable>STATUS</variable>
</infobar>
```

Types for `<default>`: `info`, `warning`, `error`, `question`, `other`.

#### grid — Table layout

```xml
<grid columns="2" row-spacing="4" column-spacing="8">
  <text><label>Name</label></text>   <entry><variable>NAME</variable></entry>
  <text><label>Email</label></text>  <entry><variable>MAIL</variable></entry>
</grid>
```

Children are placed **in flow**: in document order, left to right, wrapping every
`columns` children. Columns are **aligned from one row to the next** — something
a stack of `<hbox>` inside a `<vbox>` cannot do.

Attributes: `columns` (required in practice; without it, a single column and a
warning), `row-spacing`, `column-spacing`, `homogeneous`. Per child, as in the
boxes: `space-expand`, `space-fill`.

⚠️ **`<grid>` is not `<table>`.** `<table>` is the column list inherited from
gtkdialog: **data**. `<grid>` is a **layout**.

Variable: empty string — a container has no value of its own.

#### paned — Two areas and a handle

```xml
<paned orientation="horizontal" position="35%">
  <list><variable>FILES</variable><item>a.txt</item><item>b.txt</item></list>
  <edit><variable>CONTENT</variable></edit>
</paned>
```

Gives the **user** a way to redistribute space between two areas. `<hbox>` fixes
the split when the script is written; `<paned>` lets it move.

Attributes: `orientation` (`horizontal` by default — two areas side by side,
vertical handle; `vertical` — one above the other), `position` (initial
position, in pixels or as a `%`), `resizable="false"` (frozen handle).

⚠️ **Exactly two children.** A third is **refused with a message** on standard
error — to put more, wrap them in a `<vbox>`.

Depending on the port: `GtkPaned` (gtk3, gtk4), `QSplitter` (qt6), `Fl_Tile`
(fltk1), `elm_panes` (efl1), a draggable immediate-mode handle (sdl3). In a
terminal the handle moves with the **arrow keys** when it has focus.

Variable: empty string — a container has no value of its own.

#### levelbar — Level gauge

```xml
<levelbar range-min="0" range-max="1"><variable>LEVEL</variable><default>0.5</default></levelbar>
```

A gauge: it shows **where you stand** within a range. For progress that advances,
use `<progressbar>`; for activity with no known end, `<pulse>`.

Variable: the value, written with a decimal **dot** — never a comma, whatever the
machine's locale. A script comparing against `"0.5"` gets the same thing
everywhere.

#### drawingarea — Drawing area

```xml
<drawingarea width="200" height="120"><variable>AREA</variable></drawingarea>
```

A free surface, reserved for drawing. **Variable: empty string** — a drawing area
has no value to return.

#### wizard — A sequence of steps

```xml
<wizard>
  <vbox><text><label>Step 1</label></text></vbox>
  <vbox><text><label>Step 2</label></text></vbox>
  <variable>STEP</variable>
  <action>echo "wizard finished"</action>
</wizard>
```

Each child is a **step**; the tag builds the navigation — Back, Next, Finish.
"Finish" runs the wizard's `<action>`; it **closes nothing** by itself, that is
for the script to decide (a `<button ok>` does that).

In a terminal, the left/right arrows change step when the row has focus.
Variable: **the index of the current step**.

#### menubutton — A local menu

```xml
<menubutton>
  <menuitem><label>Copy</label><action>…</action></menuitem>
  <menuitem><label>Paste</label><action>…</action></menuitem>
  <label>Actions</label>
  <variable>CHOICE</variable>
</menubutton>
```

A menu **where you are**, not only at the top of the window like `<menubar>`. Its
children are ordinary `<menuitem>`.

⚠️ **Order matters**: the `<menuitem>` first, then `<label>` and `<variable>` —
as with `<eventbox>`.

Variable: **the label of the last item chosen**, empty before any choice.

#### stack — Pages without tabs

```xml
<stack page="0" switcher="true">
  <vbox><text><label>Page 0</label></text></vbox>
  <vbox><text><label>Page 1</label></text></vbox>
  <variable>PAGE</variable>
</stack>
```

This is `<notebook>` **without the tabs**: a single page is shown, and the script
(or the wizard) decides which. Attributes: `page` (index of the initial page,
0-based), `switcher="true"` (adds a row of numbered buttons to change page).

Variable: **the index of the visible page**, like `<notebook>`.

#### flowbox — Children that arrange themselves

```xml
<flowbox max-children-per-line="3" column-spacing="8" row-spacing="6">
  <button><label>One</label></button>
  <button><label>Two</label></button>
  <button><label>Three</label></button>
  <button><label>Four</label></button>
</flowbox>
```

Children fill a line then move on to the next. Where `<grid>` fixes the number of
columns, this one adapts to it.

Attributes: `min-children-per-line`, `max-children-per-line`, `column-spacing`,
`row-spacing`, `selection-mode` (`none` by default | `single` | `browse` |
`multiple`).

Variable: **the index of the selected child**, empty if there is none.

⚠️ **Depending on the port**: gtk3 and gtk4 genuinely reflow when the window
shrinks (GtkFlowBox). The others arrange into **N fixed columns** — same initial
rendering, without reflow. And only gtk3/gtk4 can **select**: elsewhere the
variable stays empty.

#### overlay — Stacked children

```xml
<overlay>
  <pixmap><input file>/usr/share/pixmaps/debian-logo.png</input></pixmap>
  <text><label>badge</label></text>
</overlay>
```

The **first** child is the background; the following ones float above it. It is
the only container in the language that superimposes.

⚠️ **In a terminal nothing is superimposed**: the ncurses port draws the
background then the layers one below the other, each preceded by a "· above:"
marker. The content stays readable, and the degradation is visible.

Variable: empty string — it is a container.

#### revealer — A child that shows and hides itself

```xml
<revealer transition="slide-down" duration="300" reveal="true">
  <text><label>This text appears</label></text>
</revealer>
```

Attributes: `transition` (`none` | `crossfade` | `slide-left` | `slide-right` |
`slide-up` | `slide-down`), `duration` (milliseconds), `reveal` (`true` to start
visible). `<default>true</default>` does the same thing.

⚠️ **A single child**; a second one is refused with a message.
⚠️ **The transition is only animated on gtk3 and gtk4**: the other ports show or
hide, without animation. The attribute is accepted and ignored — said rather than
left unsaid.

Variable: **`true`** or **`false`**.

#### toolbar — Action bar

```xml
<toolbar spacing="6">
  <button><label>Open</label><action>…</action></button>
  <checkbox><label>Bold</label><variable>BOLD</variable></checkbox>
</toolbar>
```

A row that **declares itself** a toolbar: the theme gives it its background and
spacing. It holds whatever you want — buttons, checkboxes, fields.

Attributes: `orientation` (`horizontal` by default | `vertical`), `spacing`.
Variable: empty string — it is a container.

#### pulse — Indeterminate activity bar

```xml
<pulse text="Downloading…"><variable>ACTIVITY</variable></pulse>
```

A bar with NO value: it says "work is happening", not "42 %". For a percentage,
use `<progressbar>`. Variable: the fixed string `pulse`.

#### eventbox — An area that catches clicks

```xml
<eventbox>
  <text><label>Click here</label></text>
  <variable>AREA</variable>
  <action>echo click</action>
</eventbox>
```

An invisible container: it wraps its content to give it a clickable area.
**Order matters** — the content first, then `<variable>` and `<action>`.
Variable: empty string (the `<action>` carries the effect).

---

## 6. Actions and signals

### 6.1 Available actions

| Action | Syntax | Description |
|--------|---------|-------------|
| `EXIT` | `EXIT:value` | Closes the window, exports EXIT=value |
| `CLOSEWINDOW` | `CLOSEWINDOW:WIDGET_NAME` | Closes the window holding the named widget. The prefix really is `closewindow`: an unknown prefix is not reported, it goes to the shell as an ordinary command. |
| `LAUNCH` | `LAUNCH:WINDOW_NAME` | Opens a new window |
| `REFRESH` | `REFRESH:WIDGET_NAME` | Re-runs a widget's `<input>` |
| `SAVE` | `SAVE:WIDGET_NAME` | Saves a widget's state |
| `CLEAR` | `CLEAR:WIDGET_NAME` | Empties a widget's content |
| `APPEND` | `APPEND:WIDGET_NAME` | Appends content to a widget |
| `FILESELECT` | `FILESELECT:VAR_NAME` | Opens a file picker |
| `ENABLE` / `DISABLE` | `ENABLE:WIDGET_NAME` | Enables/disables a widget |
| `SHOW` / `HIDE` | `SHOW:WIDGET_NAME` | Shows/hides a widget |
| `GRABFOCUS` | `GRABFOCUS:WIDGET_NAME` | Gives keyboard focus |
| `PRESENTWINDOW` | `PRESENTWINDOW:NAME` | Brings the window to the front |

### 6.2 Available signals

By default, `<action>` reacts to the widget's main signal (a click for a button).
For other signals:

```xml
<entry>
  <action signal="activate">EXIT:ok</action>          <!-- Enter key -->
  <action signal="changed">REFRESH:PREVIEW</action>   <!-- On every keystroke -->
</entry>
```

Common signals: `activate`, `changed`, `clicked`, `toggled`, `value-changed`,
`cursor-changed`, `select-row`.

### 6.3 Running a shell command

```xml
<button>
  <label>Open the browser</label>
  <action>xdg-open https://example.com</action>
</button>
```

Any action not recognised as a keyword is run as a shell command through the
core's `safe_system()`.

### 6.4 Conditional actions

```xml
<button>
  <label>Action depending on state</label>
  <action condition="command_is_true(test $BOX = 1)">REFRESH:WIDGET_A</action>
  <action condition="command_is_false(test $BOX = 1)">REFRESH:WIDGET_B</action>
</button>
```

---

## 7. Variables and input/output

### 7.1 Naming a widget

```xml
<entry><variable>MY_VALUE</variable></entry>
```

On closing, sermo emits on stdout: `MY_VALUE="text entered"`.

### 7.2 Feeding a widget from a command

```xml
<text>
  <variable>DATE_TIME</variable>
  <input>date "+%H:%M:%S"</input>
</text>
```

The command is re-run on every `REFRESH:DATE_TIME`.

### 7.3 Feeding from a file

```xml
<edit><input file>/etc/hostname</input></edit>
```

What an `<input>` reads — command or file — stops at **16 MiB**, with a warning
on standard error. The `SERMO_INPUT_MAX` environment variable changes this
limit, in bytes; `0` removes it. The progress bar is not capped. See
[SECURITY.en.md](SECURITY.en.md#size-of-what-an-input-reads).

### 7.4 Including a file of functions

```bash
sermo --include=/path/functions.sh --program=DIALOG
```

Lets you use shell functions defined in `functions.sh` inside the `<input>` and
`<action>` attributes. A relative path starts from the directory the program was
launched in.

### 7.5 An interface drawn with Glade (gtk3, gtk4)

```bash
gtk3sermo --glade-xml=interface.ui --program=main_window
```

`--glade-xml` replaces sermo's XML with a GtkBuilder interface file, the format
Glade saves. Only gtk3sermo and gtk4sermo load it, each in its own GTK version's
format; the other ports refuse the option.

- The window shown is the object whose identifier `--program` gives
  (`MAIN_WINDOW` by default).
- Every widget with an identifier becomes a variable of that name, exported on
  output like the others.
- A signal handler is not a C function but a **sermo action**:
  `<signal name="clicked" handler="exit:OK"/>`, or a shell command, subject to
  the same rules as `<action>` (`SERMO_ALLOWED_CMDS`, `--include`…).
- On `realize`, the handler's output **fills** the widget, like `<input>`.

A complete example, in GTK 3 and GTK 4: `examples/glade/`.

---

## 8. Practical examples

### 8.1 Confirmation box

> ⚠️ **`eval` and entered values.** The examples below pass the program's output
> to `eval`. That is gtkdialog's historical usage — but a field's value is
> **typed by whoever uses the dialog**, not necessarily by whoever wrote the
> script.
>
> sermo escapes the four characters the shell expands inside double quotes
> (`\`, `"`, `$` and the backtick): `eval` no longer runs them, and
> `tests/garde_echappement_sortie.sh` checks this on every push.
>
> If the dialog may be used by someone other than you, prefer
> `--do`: the values arrive through the environment and are never read back as
> code.

```bash
#!/bin/bash
CONFIRM='
<window title="Confirm" resizable="false">
  <vbox>
    <text><label>Do you want to delete this file?</label></text>
    <hbox>
      <button><label>Yes</label><action>EXIT:yes</action></button>
      <button><label>No</label><action>EXIT:no</action></button>
    </hbox>
  </vbox>
</window>'

eval "$(echo "$CONFIRM" | sermo --stdin)"
[ "$EXIT" = "yes" ] && rm "$FILE" && echo "Deleted."
```

### 8.2 A complete form with validation

```bash
#!/bin/bash
export FORM='
<window title="New profile" width="350">
  <vbox>
    <frame label="Details">
      <vbox>
        <hbox>
          <text><label>First name:</label></text>
          <entry><variable>FIRSTNAME</variable></entry>
        </hbox>
        <hbox>
          <text><label>Age:</label></text>
          <spinbutton range-min="1" range-max="120">
            <variable>AGE</variable><default>25</default>
          </spinbutton>
        </hbox>
        <hbox>
          <text><label>Country:</label></text>
          <comboboxtext>
            <variable>COUNTRY</variable>
            <item>France</item><item>Belgium</item><item>Switzerland</item>
          </comboboxtext>
        </hbox>
      </vbox>
    </frame>
    <hbox><button ok></button><button cancel></button></hbox>
  </vbox>
</window>'

sermo --program=FORM --do='
    [ "$EXIT" = "OK" ] && echo "Profile: $FIRSTNAME, $AGE years old, $COUNTRY"'
```

### 8.3 A live process monitor

```bash
#!/bin/bash
export MONITOR='
<window title="Running processes" width="600" height="400">
  <vbox>
    <tree column-header-active="true">
      <variable>PROC</variable>
      <label>PID|User|CPU%|Command</label>
      <input>ps aux --no-headers | awk '"'"'{print $2"|"$1"|"$3"|"$11}'"'"' | head -20</input>
    </tree>
    <hbox>
      <button><label>Refresh</label><action>REFRESH:PROC</action></button>
      <button><label>Close</label><action>EXIT:ok</action></button>
    </hbox>
  </vbox>
</window>'

sermo --program=MONITOR
```

---

## 9. Integrating into a shell script

### 9.1 A complete script template

```bash
#!/bin/bash
# my_app.sh — a sermo application

load_config() { cat ~/.my_app/config 2>/dev/null || echo "No config"; }
save_config() { mkdir -p ~/.my_app; echo "$1" > ~/.my_app/config; }

export INTERFACE='
<window title="My Application">
  <vbox>
    <edit><variable>CONFIG</variable><input>load_config</input></edit>
    <hbox>
      <button><label>Save</label><action>EXIT:save</action></button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>'

eval "$(sermo --include="$0" --program=INTERFACE)"

case "$EXIT" in
    save) save_config "$CONFIG"; echo "Saved." ;;
    *) echo "Cancelled." ;;
esac
```

### 9.2 Multiple windows

```bash
#!/bin/bash
export MAIN_WINDOW='
<window title="Main" name="MAIN">
  <vbox>
    <button><label>Open settings</label><action>LAUNCH:SETTINGS</action></button>
    <button><label>Quit</label><action>EXIT:quit</action></button>
  </vbox>
</window>

<window title="Settings" name="SETTINGS" visible="false">
  <vbox>
    <text><label>Settings window</label></text>
    <button><label>Close</label><action>closewindow:SETTINGS</action></button>
  </vbox>
</window>'

sermo --program=MAIN_WINDOW
```

---

## 10. FAQ and troubleshooting

**Q: The window does not appear, `Cannot open display`**
A: An SSH session without graphical forwarding. Run `export DISPLAY=:0` or
`ssh -X`.

**Q: Which backend is used when I call `sermo`?**
A: The one chosen by `update-alternatives`. To force it, call the backend's
binary: `gtk3sermo`, `qt6sermo`, `fltk1sermo`, `efl1sermo`, `sdl3sermo`,
`gtk4sermo`.

**Q: The `<terminal>` widget does not show**
A: The terminal requires VTE and exists only on `gtk3sermo` and `gtk4sermo`.
Check the binary was built with it: `gtk3sermo --version` must print "Built with
additional support for: …, VTE". On the other ports, a dialog containing
`<terminal>` still opens, without a terminal.

**Q: How do I pass large amounts of data to `<edit>`?**
A: Use `<input file>/path/file</input>` rather than a shell command.

**Q: Can I use sermo from Python?**
A: Yes. Build the XML string and pass it through
`subprocess.run(['sermo', '--stdin'], input=xml_str, text=True)`.

**Q: How do I debug an interface?**
A: Run it with `sermo --debug --program=VAR` and redirect stderr:
`sermo --program=VAR 2>debug.log`. To check the parse without a display:
`sermo --program=VAR --print-ir`.

---
