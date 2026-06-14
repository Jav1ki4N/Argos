/*=====================================*/
/* ESP-DDC                             */
/* i4N@2026                            */
/*=====================================*/
/* ESP-DDC.hpp                         */
/* All includes of .hpp files          */
/* Normally in other .cpp files this   */
/* is the only header file needed to   */
/* use ESP-DDC                         */
/*=====================================*/

#pragma once

/* Includes */
/* GPIO */
#include "general/ddc_io.hpp"

/* SPI */
#include "general/ddc_spi.hpp"
#include "general/ddc_spi_device.hpp"

/* FileSystem (must come before UI — ddc_argos_u8g2.hpp includes network.hpp which needs LFS) */
#include "thirdparty/ddc_littlefs.hpp"

/* Devices */
#include "devices/display/ddc_ssd1322_u8g2.hpp"
#include "devices/ddc_encoder.hpp"

/* Network */
#include "network/ddc_wifi.hpp"
#include "network/ddc_sntp.hpp"
#include "network/ddc_http_client.hpp"
#include "network/ddc_http_server.hpp"
#include "network/ddc_dns_server.hpp"
#include "network/ddc_mdns.hpp"
#include "network/ddc_mqtt.hpp"

