package main

import (
	"fmt"
	"strings"

	"Jav1ki4n.github.com/Argos/palette"
	tea "charm.land/bubbletea/v2"
	"charm.land/lipgloss/v2"
)

const (
	monitorTab = iota
	aboutTab
)

const (
	startOperation = iota
	stopOperation
)

var tabNames = []string{"Monitor", "About"}

const argosBanner = `    ___    ____  __________  _____
   /   |  / __ \/ ____/ __ \/ ___/
  / /| | / /_/ / / __/ / / /\__ \
 / ___ |/ _, _/ /_/ / /_/ /___/ /
/_/  |_/_/ |_|\____/\____//____/`

type tuiModel struct {
	activeTab        int
	activeOperation  int
	serviceRunning   bool
	operationMessage string
	width            int
	height           int
}

func newTUIModel() tuiModel {
	return tuiModel{
		activeTab:        monitorTab,
		activeOperation:  startOperation,
		operationMessage: "Ready.",
	}
}

func (tuiModel) Init() tea.Cmd {
	return nil
}

func (m tuiModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height

	case tea.KeyPressMsg:
		switch msg.String() {
		case "q", "ctrl+c", "esc":
			return m, tea.Quit
		case "down", "j":
			if m.activeTab == monitorTab {
				m.activeOperation = (m.activeOperation + 1) % 2
			}
		case "up", "k":
			if m.activeTab == monitorTab {
				m.activeOperation = (m.activeOperation - 1 + 2) % 2
			}
		case "enter":
			if m.activeTab == monitorTab {
				m.triggerOperation()
			}
		case "1":
			m.activeTab = monitorTab
		case "2":
			m.activeTab = aboutTab
		}
	}

	return m, nil
}

func (m *tuiModel) triggerOperation() {
	switch m.activeOperation {
	case stopOperation:
		if !m.serviceRunning {
			m.operationMessage = "Service is already stopped."
			return
		}
		m.serviceRunning = false
		m.operationMessage = "Stop triggered — service is stopped."
	default:
		if m.serviceRunning {
			m.operationMessage = "Service is already running."
			return
		}
		m.serviceRunning = true
		m.operationMessage = "Start triggered — service is running."
	}
}

func (m tuiModel) View() tea.View {
	contentWidth := max(m.width-4, 32)
	contentHeight := max(m.height-9, 5)

	tabs := make([]string, 0, len(tabNames))
	tabsWidth := 0
	for index, name := range tabNames {
		tab := renderTab(name, index == m.activeTab)
		tabs = append(tabs, tab)
		tabsWidth += lipgloss.Width(tab)
	}

	tabBar := renderTabBar(tabs, m.activeTab, contentWidth)
	body := lipgloss.NewStyle().
		Border(lipgloss.NormalBorder()).
		BorderTop(false).
		BorderForeground(lipgloss.Color(palette.Orange)).
		Width(contentWidth).
		Height(contentHeight).
		Padding(1, 2).
		Render(m.pageContent(max(contentWidth-6, 1), max(tabsWidth-4, 12)))
	statusBar := renderStatusBar(contentWidth)

	view := tea.NewView(strings.Join([]string{tabBar, body, statusBar}, "\n"))
	view.AltScreen = true
	return view
}

func renderTabBar(tabs []string, activeTab, width int) string {
	tabBar := lipgloss.JoinHorizontal(lipgloss.Top, tabs...)
	inactiveEdges := make(map[int]bool)
	activeStart := 0
	tabStart := 0
	for index, tab := range tabs {
		tabEnd := min(tabStart+lipgloss.Width(tab), width)
		if index < activeTab {
			activeStart = tabEnd
		}
		if index != activeTab && tabStart < width {
			inactiveEdges[tabStart] = true
			inactiveEdges[max(tabEnd-1, tabStart)] = true
		}
		tabStart = tabEnd
	}
	activeEnd := min(activeStart+lipgloss.Width(tabs[activeTab]), width)

	var baseline strings.Builder
	for column := 0; column < width; column++ {
		switch {
		case column == activeStart && activeStart == 0:
			baseline.WriteString("│")
		case column == activeStart:
			baseline.WriteString("┘")
		case column == activeEnd-1 && activeEnd == width:
			baseline.WriteString("│")
		case column == activeEnd-1:
			baseline.WriteString("└")
		case column > activeStart && column < activeEnd-1:
			baseline.WriteString(" ")
		case inactiveEdges[column] && column == 0:
			baseline.WriteString("├")
		case inactiveEdges[column]:
			baseline.WriteString("┴")
		case column == width-1:
			baseline.WriteString("┐")
		default:
			baseline.WriteString("─")
		}
	}

	styledBaseline := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Orange)).
		Render(baseline.String())

	return tabBar + "\n" + styledBaseline
}

func renderStatusBar(width int) string {
	testing := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Black)).
		Background(lipgloss.Color(palette.Orange)).
		Bold(true).
		Padding(0, 1).
		Render(tool_id)

	version := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Black)).
		Background(lipgloss.Color(palette.Yellow)).
		Bold(true).
		Padding(0, 1).
		Render(tool_version)

	remainingWidth := max(width-lipgloss.Width(testing)-lipgloss.Width(version), 0)
	help := "1/2 page • ↑/↓ operation • ↵ run • q quit "
	if lipgloss.Width(help) > remainingWidth {
		help = "↑/↓ operation • ↵ run • q quit "
	}
	if lipgloss.Width(help) > remainingWidth {
		help = "q quit "
	}

	remainder := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.White)).
		Background(lipgloss.Color(palette.Gray)).
		Faint(true).
		Align(lipgloss.Right).
		Width(remainingWidth).
		Render(help)

	return lipgloss.JoinHorizontal(lipgloss.Top, testing, version, remainder)
}

func renderTab(label string, active bool) string {
	style := lipgloss.NewStyle().
		Border(lipgloss.RoundedBorder()).
		BorderBottom(false).
		Padding(0, 2)

	if active {
		style = style.
			Bold(true).
			Foreground(lipgloss.Color(palette.White)).
			BorderForeground(lipgloss.Color(palette.Orange))
	} else {
		style = style.
			Faint(true).
			BorderForeground(lipgloss.Color(palette.Orange))
	}

	return style.Render(label)
}

func (m tuiModel) pageContent(width, dividerColumn int) string {
	switch m.activeTab {
	case aboutTab:
		return renderAboutPage(width)
	default:
		return m.renderMonitorPage(width, dividerColumn)
	}
}

func (m tuiModel) renderMonitorPage(width, dividerColumn int) string {
	leftWidth := min(max(dividerColumn-1, 11), max(width-12, 11))
	rightWidth := max(width-leftWidth-3, 8)

	operationsTitle := lipgloss.NewStyle().
		Bold(true).
		Width(leftWidth).
		Align(lipgloss.Center).
		Render("Operations")
	operations := strings.Join([]string{
		renderOperationButton("Start", m.activeOperation == startOperation, leftWidth),
		renderOperationButton("Stop", m.activeOperation == stopOperation, leftWidth),
	}, "\n")

	state := "Stopped"
	if m.serviceRunning {
		state = "Running"
	}
	logTitle := lipgloss.NewStyle().
		Bold(true).
		Width(rightWidth).
		Align(lipgloss.Center).
		Render("Log")
	information := strings.Join([]string{
		fmt.Sprintf("Service: %s", state),
		lipgloss.NewStyle().Faint(true).Render(m.operationMessage),
	}, "\n")
	information = lipgloss.NewStyle().Width(rightWidth).Render(information)

	dividerStyle := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Orange)).
		Bold(true)
	header := lipgloss.JoinHorizontal(
		lipgloss.Top,
		operationsTitle,
		"   ",
		logTitle,
	)

	horizontalLine := []rune(strings.Repeat("─", width))
	if len(horizontalLine) > 0 {
		horizontalLine[0] = ' '
		horizontalLine[len(horizontalLine)-1] = ' '
	}
	horizontalDivider := dividerStyle.Render(string(horizontalLine))

	content := lipgloss.JoinHorizontal(
		lipgloss.Top,
		operations,
		"   ",
		information,
	)

	return strings.Join([]string{header, horizontalDivider, content}, "\n")
}

func renderOperationButton(label string, selected bool, width int) string {
	style := lipgloss.NewStyle().Width(width).Align(lipgloss.Center)

	if selected {
		style = style.
			Foreground(lipgloss.Color(palette.Orange)).
			Bold(true)
	} else {
		style = style.Faint(true)
	}

	return style.Render(label)
}

func renderAboutPage(width int) string {
	banner := renderGradient(argosBanner, palette.Orange, palette.Yellow)
	repository := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Yellow)).
		Underline(true).
		Hyperlink("https://github.com/Jav1ki4N/Argos").
		Render("github.com/Jav1ki4N/Argos")
	creditsTitle := lipgloss.NewStyle().Bold(true).Render("Credits")
	technologies := lipgloss.NewStyle().
		Foreground(lipgloss.Color(palette.Orange)).
		Bold(true).
		Render("Go ❋ Bubble Tea ❋ Lip Gloss")
	paletteTable := renderPaletteTable(width)

	return fmt.Sprintf(
		"%s\n\n%s %s · %s\nTerminal services controller for Argos.\n%s\n\n%s\nPowered by %s\n%s",
		banner,
		tool_id,
		tool_version,
		make_copyrigt(),
		repository,
		creditsTitle,
		technologies,
		paletteTable,
	)
}

func renderPaletteTable(width int) string {
	colors := []string{
		palette.Gray,
		palette.LightGray,
		palette.White,
		palette.Orange,
		palette.Yellow,
	}

	swatches := make([]string, 0, len(colors))
	for _, color := range colors {
		swatches = append(swatches, lipgloss.NewStyle().
			Foreground(lipgloss.Color(color)).
			Render("██"))
	}

	strip := lipgloss.JoinHorizontal(lipgloss.Top, swatches...)
	return lipgloss.NewStyle().Width(width).Align(lipgloss.Right).Render(strip)
}

func renderGradient(text, startHex, endHex string) string {
	var startRed, startGreen, startBlue int
	var endRed, endGreen, endBlue int
	if _, err := fmt.Sscanf(startHex, "#%02x%02x%02x", &startRed, &startGreen, &startBlue); err != nil {
		return text
	}
	if _, err := fmt.Sscanf(endHex, "#%02x%02x%02x", &endRed, &endGreen, &endBlue); err != nil {
		return text
	}

	lines := strings.Split(text, "\n")
	maxWidth := 1
	for _, line := range lines {
		maxWidth = max(maxWidth, lipgloss.Width(line))
	}

	var gradient strings.Builder
	for lineIndex, line := range lines {
		for column, character := range []rune(line) {
			if character == ' ' {
				gradient.WriteRune(character)
				continue
			}

			position := float64(column) / float64(max(maxWidth-1, 1))
			red := startRed + int(float64(endRed-startRed)*position)
			green := startGreen + int(float64(endGreen-startGreen)*position)
			blue := startBlue + int(float64(endBlue-startBlue)*position)
			color := lipgloss.Color(fmt.Sprintf("#%02X%02X%02X", red, green, blue))
			gradient.WriteString(lipgloss.NewStyle().Foreground(color).Bold(true).Render(string(character)))
		}
		if lineIndex < len(lines)-1 {
			gradient.WriteByte('\n')
		}
	}

	return gradient.String()
}
