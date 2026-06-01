/*
Copyright (c) 2026 i4N <https://github.com/Jav1ki4N/Argos>
*/
package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:   "argos",
	Short: "Launch Argos server to monitor and control your ESP32 devices",
	Long: `
argos [-v | --version] [-h | --help | help] <command>

===========================================================================

flags:
	-v, --version Show the current version of Argos
	-h, --help    Show this help message

commands:
	start[-v | --verbose] Start the Argos server to monitor and control your
	ESP32 devices. Use -v or --verbose for detailed logging.

===========================================================================
`,
	Run: func(cmd *cobra.Command, args []string) {
		bold := "\x1b[1m"
		cyan := "\x1b[38;2;0;173;216m"
		reset := "\x1b[0m"

		fmt.Print(cyan + bold +
			"                       ___                         \n" +
			"                      /   |  _________ _____  _____\n" +
			"                     / /| | / ___/ __ `/ __ \\/ ___/\n" +
			"                    / ___ |/ /  / /_/ / /_/ (__  ) \n" +
			"                   /_/  |_/_/   \\__, /\\____/____/  \n" +
			"                               /____/              \n" +
			"\n" +
			"===========================================================================\n" +
			"     2026 @ i4N  https://github.com/Jav1ki4N/Argos | Version: Prototype\n" +
			"\n" +
			reset)
	},
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the rootCmd.
func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

func init() {
	rootCmd.Flags().BoolP("version", "v", false, "Show version")
	rootCmd.SetHelpTemplate(`{{with .Long}}{{.}}{{end}}`)
}
