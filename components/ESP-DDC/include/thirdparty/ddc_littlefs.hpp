// git submodule add https://github.com/joltwallet/esp_littlefs.git
// git submodule update --init --recursive

/* Partition Info */
//littlefs,     data, littlefs,  0x2A0000, 0x160000

#pragma once


#include "esp_log.h"
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <dirent.h>

/* Third-Party */
#include "../../components/esp_littlefs/include/esp_littlefs.h"


class LFS
{
    using conf_t = esp_vfs_littlefs_conf_t;
    public:
    LFS(conf_t config = default_config)
    : _config(config)
    {
        esp_err_t err = esp_vfs_littlefs_register(&config);
        if (err != ESP_OK) {
            ESP_LOGE("LFS", "Failed to register LittleFS");
            return;
        }
        mounted = true;
    }
    ~LFS()
    {
        if (mounted) {
            esp_vfs_littlefs_unregister("littlefs");
        }
    }

    //  Func    mkdir
    /// @brief  Create a directory at the specified path within LittleFS
    /// @note   The path is relative to the LittleFS base path
    ///         e.g. mkdir("/web") creates /lfs/web if base_path is "/lfs"
    esp_err_t mkdir(const char* path)
    {
        std::string full_path = std::string(_config.base_path) + path;
        if (!mounted) return ESP_ERR_INVALID_STATE;
        if (::mkdir(full_path.c_str(), 0755) != 0) {
            if (errno == EEXIST) return ESP_OK;
            ESP_LOGE(TAG, "mkdir failed: %s/n", full_path.c_str());
            ESP_LOGE(TAG, "Error: %s", strerror(errno));
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    esp_err_t info(size_t &total, size_t &used)
    {
        if (!mounted) return ESP_ERR_INVALID_STATE;
        return esp_littlefs_info(_config.partition_label, 
                                 &total, 
                                 &used);
    }

    FILE* read(const char* path)
    {
        if (!mounted) return nullptr;
        std::string full = std::string(_config.base_path) + path;
        return fopen(full.c_str(), "r");
    }

    //  Func    write
    /// @brief  Write data to a file (create or truncate + write)
    /// @note   Works for any file type — HTML text, binary, etc.
    ///         All files are raw bytes at the filesystem level.
    /// @param  path — relative path within LittleFS (e.g. "portal.html")
    /// @param  data — pointer to the data buffer
    /// @param  len  — number of bytes to write
    esp_err_t write(const char* path, const void* data, size_t len)
    {
        if (!mounted) return ESP_ERR_INVALID_STATE;

        std::string full = std::string(_config.base_path) + path;
        FILE *f = fopen(full.c_str(), "w");
        if (!f) {
            ESP_LOGE(TAG, "write: fopen failed for %s", full.c_str());
            return ESP_FAIL;
        }

        size_t written = fwrite(data, 1, len, f);
        fclose(f);

        if (written != len) {
            ESP_LOGE(TAG, "write: wrote %zu/%zu bytes to %s", written, len, full.c_str());
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    //  Func    overwrite
    /// @brief  Same as write() — fopen "w" already truncates existing files
    /// @note   Kept as a separate API in case append-mode write is added later
    esp_err_t overwrite(const char* path, const void* data, size_t len)
    {
        return write(path, data, len);
    }

    esp_err_t remove(const char* path)
    {
        if (!mounted) return ESP_ERR_INVALID_STATE;
        std::string full = std::string(_config.base_path) + path;
        if (std::remove(full.c_str()) != 0) {
            ESP_LOGE(TAG, "Failed to remove file: %s", full.c_str());
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    esp_err_t rmdir(const char* path)
    {
        if (!mounted) return ESP_ERR_INVALID_STATE;
        std::string full = std::string(_config.base_path) + path;
        if (::rmdir(full.c_str()) != 0) {
            ESP_LOGE(TAG, "Failed to remove dir: %s", full.c_str());
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    std::string_view base() const { return _config.base_path; }

    private:

    const char* TAG = "LFS";
    bool mounted = false;

    static inline const conf_t default_config =
    {
        .base_path = "/lfs",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .read_only = false,
        .dont_mount = false,
        .grow_on_mount = true
    };

    conf_t _config = {};
};







