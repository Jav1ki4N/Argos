/*=====================================*/
/* ESP-DDC                             */
/* i4N@2026                            */
/*=====================================*/
/* ddc_littlefs.hpp                    */
/* RAII wrapper for esp_littlefs       */
/* - Auto mount/unmount                */
/* - POSIX file I/O helpers            */
/* - Partition info query              */
/*=====================================*/

#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_littlefs.h"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

class LittleFS
{
  public:
    LittleFS(const char *base_path = "/littlefs",
             const char *partition_label = "littlefs",
             bool format_if_mount_failed = true)
    {
        esp_vfs_littlefs_conf_t conf = {
            .base_path = base_path,
            .partition_label = partition_label,
            .partition = nullptr,
            .format_if_mount_failed = format_if_mount_failed,
            .read_only = false,
            .dont_mount = false,
            .grow_on_mount = false,
        };

        esp_err_t ret = esp_vfs_littlefs_register(&conf);

        if (ret == ESP_OK)
        {
            _mounted = true;
            strlcpy(_partition_label, partition_label, sizeof(_partition_label));
            strlcpy(_base_path, base_path, sizeof(_base_path));
            ESP_LOGI(TAG, "LittleFS mounted at [%s]", _base_path);
        }
        else
        {
            _mounted = false;
            if (ret == ESP_FAIL)
                ESP_LOGE(TAG, "Failed to mount or format filesystem");
            else if (ret == ESP_ERR_NOT_FOUND)
                ESP_LOGE(TAG, "Partition [%s] not found in partition table", partition_label);
            else
                ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
    }

    ~LittleFS()
    {
        if (_mounted)
        {
            esp_vfs_littlefs_unregister(_partition_label);
            ESP_LOGI(TAG, "LittleFS unmounted");
        }
    }

    /* ---- Status ---- */

    bool is_mounted() const
    {
        return _mounted;
    }

    /* ---- Partition Info ---- */

    bool info(size_t &total, size_t &used) const
    {
        if (!_mounted) return false;
        esp_err_t ret = esp_littlefs_info(_partition_label, &total, &used);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get partition info (%s)", esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    /* ---- File Helpers ---- */

    /**
     * @brief  Build a full path under the mount point.
     * @param  filename  Relative filename (e.g. "config.json")
     * @return Stack-allocated full path (e.g. "/littlefs/config.json")
     */
    const char *path(const char *filename) const
    {
        static char buf[128];
        snprintf(buf, sizeof(buf), "%s/%s", _base_path, filename);
        return buf;
    }

    bool file_exists(const char *filename) const
    {
        struct stat st;
        return stat(path(filename), &st) == 0;
    }

    bool file_delete(const char *filename) const
    {
        if (unlink(path(filename)) != 0)
        {
            ESP_LOGE(TAG, "Failed to delete [%s]", filename);
            return false;
        }
        return true;
    }

    /**
     * @brief  Read entire file into a caller-provided buffer.
     * @return Number of bytes read, or -1 on error.
     */
    int file_read(const char *filename, char *buf, size_t buf_size) const
    {
        FILE *f = fopen(path(filename), "r");
        if (!f)
        {
            ESP_LOGE(TAG, "Failed to open [%s] for reading", filename);
            return -1;
        }
        size_t n = fread(buf, 1, buf_size - 1, f);
        fclose(f);
        buf[n] = '\0';
        return (int)n;
    }

    /**
     * @brief  Write data to a file (overwrites if exists).
     * @return true on success.
     */
    bool file_write(const char *filename, const char *data) const
    {
        FILE *f = fopen(path(filename), "w");
        if (!f)
        {
            ESP_LOGE(TAG, "Failed to open [%s] for writing", filename);
            return false;
        }
        fputs(data, f);
        fclose(f);
        return true;
    }

  private:
    static constexpr const char *TAG = "littlefs";
    char _base_path[32]{};
    char _partition_label[32]{};
    bool _mounted = false;
};
