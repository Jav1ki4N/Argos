/*
Copyright © 2026 NAME HERE <EMAIL ADDRESS>
*/
package cmd

import (
	/* System */
	"encoding/json"
	"log"
	"net"
	"net/http"
	"strings"
	"time"

	/* Libs */
	"github.com/grandcat/zeroconf"
	"github.com/shirou/gopsutil/v4/cpu"
	"github.com/shirou/gopsutil/v4/disk"
	"github.com/shirou/gopsutil/v4/host"
	"github.com/shirou/gopsutil/v4/mem"
	"github.com/shirou/gopsutil/v4/sensors"
	"github.com/spf13/cobra"
	/* Cobra command */)

var verbose bool

// startCmd represents the start command
var startCmd = &cobra.Command{
	Use:   "start",
	Short: "Start the Argos server",
	Long: `
Usage: argos start [-v | --verbose]
Start the Argos server to monitor system metrics and expose an API
for ESP32. Use -v or --verbose for detailed logging.

Before starting the server, please make sure that:
[1] Your machine is connected to a Wi-Fi network
[2] There's no VPN service running that could interfere with mDNS broadcasting

`,
	Run: func(cmd *cobra.Command, args []string) {
		verbose, _ = cmd.Flags().GetBool("verbose")
		startServer()
	},
}

/* Get local IP of this machine */
func getLocalIP() string {
	conn, err := net.Dial("udp", "8.8.8.8:53")
	if err != nil {
		return "Failed to get local IP"
	}
	defer conn.Close()
	return conn.LocalAddr().(*net.UDPAddr).IP.String()
}

/* Get CPU Temp */
func getCPUTemp() float64 {
	temp, err := sensors.SensorsTemperatures()
	if len(temp) == 0 { // real error
		println("Failed to get CPU temperature")
		return 0
	}
	if err != nil {
		println("") // let it be
	}
	temp_keys := []string{"coretemp", "k10temp", "cpu", "cpu_thermal", "soc", "package"}

	/* Key matching */
	for _, key := range temp_keys {
		for _, t := range temp {
			if t.Temperature > 0 && strings.Contains(strings.ToLower(t.SensorKey), key) {
				return t.Temperature
			}
		}
	}

	/* Fallback to first valid reading */
	for _, t := range temp {
		if t.Temperature > 0 {
			return t.Temperature
		}
	}

	return 0
}

/* System information typedef */
type Info struct {
	CPUPercent float64 `json:"cpu_percent"`
	CPUCores   int     `json:"cpu_cores"`
	CPUThreads int     `json:"cpu_threads"`
	CPUFreqMHz float64 `json:"cpu_freq_mhz"`
	CPUTemp    float64 `json:"cpu_temp"`

	MemTotalMB uint64  `json:"mem_total_mb"`
	MemUsedMB  uint64  `json:"mem_used_mb"`
	MemPercent float64 `json:"mem_percent"`

	DiskTotalGB float64 `json:"disk_total_gb"`
	DiskUsedGB  float64 `json:"disk_used_gb"`
	DiskPercent float64 `json:"disk_percent"`

	OS       string `json:"os"`
	OSVer    string `json:"os_version"`
	HostName string `json:"host_name"`
	UptimeS  uint64 `json:"uptime_s"`
}

/* Get system infos */
func handler(w http.ResponseWriter, r *http.Request) {
	/* CPU */
	cpuPercent, _ := cpu.Percent(500*time.Millisecond, false) // read every 500ms
	cores, _ := cpu.Counts(false)                             // CPU cores
	threads, _ := cpu.Counts(true)                            // CPU threads
	info, _ := cpu.Info()                                     // CPU info
	freq := 0.0
	if len(info) > 0 {
		freq = info[0].Mhz
	}
	cpuPct := 0.0
	if len(cpuPercent) > 0 {
		cpuPct = cpuPercent[0]
	}

	/* Memory, Disk & host */
	vmem, _ := mem.VirtualMemory()
	disk, _ := disk.Usage("/")
	host, _ := host.Info()

	data := Info{
		CPUPercent: cpuPct,
		CPUCores:   cores,
		CPUThreads: threads,
		CPUFreqMHz: freq,
		CPUTemp:    getCPUTemp(),

		MemTotalMB: vmem.Total / (1024 * 1024),
		MemUsedMB:  vmem.Used / (1024 * 1024),
		MemPercent: vmem.UsedPercent,

		DiskTotalGB: float64(disk.Total) / 1e9,
		DiskUsedGB:  float64(disk.Used) / 1e9,
		DiskPercent: disk.UsedPercent,

		OS:       host.OS,
		OSVer:    host.PlatformVersion,
		HostName: host.Hostname,
		UptimeS:  host.Uptime,
	}

	if verbose {
		pretty, _ := json.MarshalIndent(data, "", "  ")
		log.Println("[Argos]: Collected system info:\n" + string(pretty))
	}

	json.NewEncoder(w).Encode(data)
}

func startServer() {
	/* get local IP */
	ip := getLocalIP()

	/* Register mDNS service */
	server, err := zeroconf.RegisterProxy(
		"argos",
		"_http._tcp.",
		"local.",
		8080,
		"argos-target",
		[]string{ip},
		[]string{"path=/api/info"},
		nil,
	)
	if err != nil {
		log.Fatal(err)
	}
	defer server.Shutdown()

	/* Info */
	/* Basic */
	log.Printf("[Argos]: mDNS broadcasting: argos-target.local → %s:8080\n", ip)
	log.Printf("[Argos]: Service is started. You may connect your ESP32 device to this server\n")
	log.Printf("[Argos]: This process could fail if you are using a VPN, it's advised to launch the server before connecting to a VPN\n")
	log.Printf("[Argos]: Press Ctrl+C to stop the server\n")

	/* Http Handler */
	http.HandleFunc("/api/info", handler)
	log.Fatal(http.ListenAndServe(":8080", nil))
}

func init() {
	rootCmd.AddCommand(startCmd)
	startCmd.SetHelpTemplate(`{{with .Long}}{{.}}{{end}}`)
	startCmd.Flags().BoolP("verbose", "v", false, "Enable verbose logging")
}
