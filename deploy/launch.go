package main

import (
	"encoding/json"
	"log"
	"net"
	"net/http"
	"strings"
	"time"

	"github.com/grandcat/zeroconf"
	"github.com/shirou/gopsutil/v4/cpu"
	"github.com/shirou/gopsutil/v4/disk"
	"github.com/shirou/gopsutil/v4/host"
	"github.com/shirou/gopsutil/v4/mem"
	"github.com/shirou/gopsutil/v4/sensors"
)

func getLocalIP() string {
	conn, err := net.Dial("udp", "8.8.8.8:53")
	if err != nil {
		return ""
	}
	defer conn.Close()
	return conn.LocalAddr().(*net.UDPAddr).IP.String()
}

func getCPUTemp() float64 {
	temps, err := sensors.SensorsTemperatures()
	if len(temps) == 0 {
		return 0
	}
	if err != nil {
		// gopsutil may return warnings as an error while still providing valid readings.
		// log.Printf("temperature probe warning: %v", err)
	}

	for _, key := range []string{"coretemp", "k10temp", "cpu", "cpu_thermal", "soc", "package"} {
		for _, t := range temps {
			if t.Temperature > 0 && strings.Contains(strings.ToLower(t.SensorKey), key) {
				return t.Temperature
			}
		}
	}

	// Fallback to the first non-zero reading if no preferred key matched.
	for _, t := range temps {
		if t.Temperature > 0 {
			return t.Temperature
		}
	}

	return 0
}

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

func handler(w http.ResponseWriter, r *http.Request) {
	cpuPercent, _ := cpu.Percent(500*time.Millisecond, false)
	cores, _ := cpu.Counts(false)
	threads, _ := cpu.Counts(true)
	info, _ := cpu.Info()
	freq := 0.0
	if len(info) > 0 {
		freq = info[0].Mhz
	}
	cpuPct := 0.0
	if len(cpuPercent) > 0 {
		cpuPct = cpuPercent[0]
	}

	vm, _ := mem.VirtualMemory()
	du, _ := disk.Usage("/")
	hi, _ := host.Info()

	json.NewEncoder(w).Encode(Info{
		CPUPercent: cpuPct,
		CPUCores:   cores,
		CPUThreads: threads,
		CPUFreqMHz: freq,
		CPUTemp:    getCPUTemp(),

		MemTotalMB: vm.Total / (1024 * 1024),
		MemUsedMB:  vm.Used / (1024 * 1024),
		MemPercent: vm.UsedPercent,

		DiskTotalGB: float64(du.Total) / 1e9,
		DiskUsedGB:  float64(du.Used) / 1e9,
		DiskPercent: du.UsedPercent,

		OS:       hi.OS,
		OSVer:    hi.PlatformVersion,
		HostName: hi.Hostname,
		UptimeS:  hi.Uptime,
	})
}

func main() {
	ip := getLocalIP()

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
	log.Printf("mDNS broadcasting: argos-target.local → %s:8080", ip)

	http.HandleFunc("/api/info", handler)
	log.Fatal(http.ListenAndServe(":8080", nil))
}
