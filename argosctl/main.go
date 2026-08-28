package main

import (
	"fmt"
	"os"
	"time"

	"Jav1ki4n.github.com/Argos/palette"
	tea "charm.land/bubbletea/v2" // TUI framework
	"charm.land/lipgloss/v2"      // Style
)

const (
	/* Tool metadata */
	tool_version = "v0.1"
	tool_id      = "argosctl"
	tool_series  = "Argos"
	tool_author  = "Ian Javik"
)

func make_copyrigt() string {
	time := time.Now().Year()
	return fmt.Sprintf("%s (C) %d %s", tool_series, time, tool_author)
}

func main() {
	defer onExit()

	if _, err := tea.NewProgram(newTUIModel()).Run(); err != nil {
		fmt.Fprintf(os.Stderr, "failed to launch TUI: %v\n", err)
		os.Exit(1)
	}
}

func onExit() {
	exit_info_prompt := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Black)).
		Background(lipgloss.Color(palette.Orange)).
		Padding(0, 1).
		Bold(true)

	exit_info := lipgloss.NewStyle()

	ext_info := lipgloss.JoinHorizontal(lipgloss.Center, exit_info_prompt.Render("Exit"), " ", exit_info.Render(make_copyrigt()))

	fmt.Println()
	fmt.Println(ext_info)
}
