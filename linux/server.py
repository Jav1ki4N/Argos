from   flask import Flask, jsonify
import psutil
import platform
import socket
import time

# server object
app = Flask(__name__)

def get_cpu_temp():
    try:
        temps = psutil.sensors_temperatures()
        if not temps:
            return None
        for key in ['coretemp', 'k10temp', 'acpitz', 'cpu_thermal']:
            if key in temps:
                return round(temps[key][0].current, 1)
    except Exception:
        pass
    return None

@app.route('/api/info')
def system_info():
    cpu_freq = psutil.cpu_freq()
    mem      = psutil.virtual_memory()
    disk     = psutil.disk_usage('/')
    
    return jsonify({
        # CPU
        "cpu_percent":   psutil.cpu_percent(interval=0.5),
        "cpu_cores":     psutil.cpu_count(logical=False),
        "cpu_threads":   psutil.cpu_count(logical=True),
        "cpu_freq_mhz":  round(cpu_freq.current) if cpu_freq else 0,
        "cpu_temp":      get_cpu_temp(),
        # 内存
        "mem_total_mb":  mem.total // (1024*1024),
        "mem_used_mb":   mem.used  // (1024*1024),
        "mem_percent":   mem.percent,
        # 磁盘
        "disk_total_gb": round(disk.total / 1e9, 1),
        "disk_used_gb":  round(disk.used  / 1e9, 1),
        # 系统
        "os":            platform.system(),
        "os_version":    platform.version()[:40],
        "host_name":     socket.gethostname(),
        "uptime_s":      int(time.time() - psutil.boot_time()),
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)