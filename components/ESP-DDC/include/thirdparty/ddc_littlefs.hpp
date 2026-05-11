// git submodule add https://github.com/joltwallet/esp_littlefs.git
// git submodule update --init --recursive

/* Partition Info */
//littlefs,     data, littlefs,  0x2A0000, 0x160000

#pragma once


#include "esp_log.h"
#include <sys/stat.h>

#include <string>

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

    esp_err_t mkdir(const char* path)
    {
        std::string full_path = std::string(_config.base_path) + "/" + path;
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

    const char* base() const { return _config.base_path; }

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







