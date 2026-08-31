# argosctl

**argosctl** is an experimental terminal UI for the Argos project.

> **Status: UI prototype.** The Monitor page and Start/Stop interactions are implemented, but the operations currently change only the TUI's in-memory state. They do not start, stop, inspect, or supervise the real agent in `../deploy/`.

## Build and run

```bash
go build -o argosctl .
./argosctl
```

Or run it directly:

```bash
go run .
```

Controls:

- `1` / `2`: switch between Monitor and About.
- Arrow keys or `j` / `k`: select Start or Stop.
- `Enter`: trigger the selected prototype operation.
- `q`, `Esc`, or `Ctrl+C`: quit.

Before this can be called a service controller, it needs process lifecycle integration with the `deploy` agent, real status/log reporting, error handling, and tests.

## Credits

TUI built with [**bubbletea**](https://github.com/charmbracelet/bubbletea) & [**lipgloss**](https://github.com/charmbracelet/lipgloss) by [**charm**](https://charm.land/).

Powered by [**Go**](https://go.dev/).

<div align = "center">
    <img src = "assets/go-gopher.png" width=75 alt="go-gopher">
</div>
