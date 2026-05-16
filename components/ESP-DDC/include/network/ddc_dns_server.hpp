
#pragma once

/* ESP-IDF Components */
#include "esp_netif.h"
#include "esp_log.h"

/* lwIP */
#include "lwip/sockets.h"
#include "lwip/inet.h"

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* C/C++ Libraries */
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <array>

class DNServer
{
public:
    DNServer() = default;
    ~DNServer() = default;

    void start()
    {
        isRunning = true;
        xTaskCreate([](void* arg) {
            static_cast<DNServer*>(arg)->run();
            vTaskDelete(nullptr);
        }, "dns_server", 4096, this, 5, &dns_task_handle);
    }

    void stop()
    {
        isRunning = false;
        if (dns_task_handle) {
            vTaskDelete(dns_task_handle);
            dns_task_handle = nullptr;
        }
    }

private:
    static constexpr const char* TAG = "DNServer";
    /* DNS Port */
    static constexpr uint8_t DNS_PORT = 53;

    /* DNS Packet Structures */
    // DNS Header Packet
    struct [[gnu::packed]] DNSHeader_t
    {
        uint16_t id;       // DNS request/question ID
        uint16_t flags;    // DNS flags, many bit fields packed in
        uint16_t qd_count; // how many questions in this (if is request) request
        uint16_t an_count; // how many answers in this (if is response) response
        uint16_t ns_count; // authority record count
        uint16_t ar_count; // additional record count
    };

    // DNS Question Packet
    struct [[gnu::packed]] DNSQuestion_t 
    {
        uint16_t type;
        uint16_t dns_class;
    };

    // DNS Answer Packet
    struct [[gnu::packed]] DNSAnswer_t
    {
        uint16_t ptr_offset; // pointer to question/domain name
                             // in DNS packet, for compression
        uint16_t type;
        uint16_t dns_class;
        uint32_t ttl;        // time to live (of the response cached)
        uint16_t addr_len;   // length of the IP address (4 for IPv4)
        uint32_t ip_addr;    // IP address
    };
    
    /* request */
    // bitmask
    static constexpr uint16_t OPCODE_MASK = 0x7800; // opcode bitmask in DNS header (multi bits)
    static constexpr uint16_t QR_RESPONSE_FLAG = 1 << 7;     // response flag bitmask in DNS header
    // request type
    static constexpr uint16_t QD_TYPE_A = 0x0001;   // request type: IPV4 address

    std::vector<uint8_t> response_packet_buffer;

    std::pair<std::string, uint8_t*> ParseName(uint8_t* raw)
    {
        std::string parsed;
        while (*raw != 0) {
            uint8_t len = *raw++;
            parsed.append(reinterpret_cast<const char*>(raw), len);
            parsed.push_back('.');
            raw += len;
        }
        if (!parsed.empty()) parsed.pop_back();
        return {parsed, raw + 1}; 
    }

    /* Response Packet Builder  - Captive Portal Version */
    int BuildResponse_CP(const uint8_t* packet,
                      size_t         packet_len,
                      size_t         max_response_len)
    {
        /* Calculate response length */
        size_t qd_count_raw =ntohs(reinterpret_cast<const DNSHeader_t*>(packet)->qd_count);
        size_t response_len = packet_len + qd_count_raw * sizeof(DNSAnswer_t);
        if (response_len > max_response_len) {
            ESP_LOGE(TAG, "Response length %d exceeds max allowed %d", response_len, max_response_len);
            return -1;
        }
        /* Resize buffer */
        response_packet_buffer.clear();
        response_packet_buffer.resize(response_len);

        /* Write packet data to buffer */
        std::memcpy(response_packet_buffer.data(), packet, packet_len);
        uint8_t* header_start = response_packet_buffer.data();

        /* Parse DNS header */
        DNSHeader_t* header = reinterpret_cast<DNSHeader_t*>(header_start);
        ESP_LOGD(TAG, "DNS request with header id: 0x%X, flags: 0x%X, qd_count: %d",
                 ntohs(header->id), ntohs(header->flags), ntohs(header->qd_count));

        /* Check if it's a standard query */
        if ((header->flags & OPCODE_MASK) != 0) return 0;
        
        /* Build response */
        // build header: Set QR flag to mark as response
        header->flags |= QR_RESPONSE_FLAG;
        
        // build header: set answer count same as question count
        // answer all questions
        uint16_t qd_count = ntohs(header->qd_count);
        header->an_count = htons(qd_count);

        //build answer: pointer to question & answer
        uint8_t* answer_start   = header_start + packet_len; // answer starts right after question
        uint8_t* curr_question = header_start + sizeof(DNSHeader_t); // question

        //build answer: respond to all questions
        for(uint8_t question_index = 0;
                    question_index < qd_count;
                    question_index++)
        {
            auto [name, next_ptr] = ParseName(curr_question);
            DNSQuestion_t* question = reinterpret_cast<DNSQuestion_t*>(next_ptr);
            uint16_t q_type  = ntohs(question->type);      // question type
            uint16_t q_class = ntohs(question->dns_class);
            
            /* If question type is A (IPv4 address) */
            // query for any domain will be redirected to captive portal IP, i.e. ESP32 AP IP
            if (q_type == QD_TYPE_A) {
                // for all rules
                esp_netif_ip_info_t ip_info;
                esp_netif_get_ip_info(
                    esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

                // build answer
                DNSAnswer_t* answer = reinterpret_cast<DNSAnswer_t*>(answer_start);
                answer->ptr_offset = htons(0xC000 | (curr_question - header_start));
                answer->type       = htons(q_type);
                answer->dns_class  = htons(q_class);
                answer->ttl        = htonl(300);
                answer->addr_len   = htons(sizeof(ip_info.ip.addr));
                answer->ip_addr    = ip_info.ip.addr;

                answer_start += sizeof(DNSAnswer_t);
            }
            curr_question = next_ptr + sizeof(DNSQuestion_t);
        }
        return response_len;
    }

    void run()
    {
        std::array<uint8_t, DNS_MAX_LEN> rx_buffer;

        while (isRunning) {
            struct sockaddr_in dest_addr = {};
            dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr.sin_family      = AF_INET;
            dest_addr.sin_port        = htons(DNS_PORT);

            int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG, "DNS socket created");

            if (bind(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
                ESP_LOGE(TAG, "Socket bind failed: errno %d", errno);
                close(sock);
                break;
            }
            ESP_LOGI(TAG, "DNS socket bound on port %d", DNS_PORT);

            while (isRunning) {
                struct sockaddr_in6 source_addr = {};
                socklen_t socklen = sizeof(source_addr);

                int len = recvfrom(sock,
                                rx_buffer.data(), rx_buffer.size() - 1,
                                0,
                                (struct sockaddr*)&source_addr, &socklen);
                if (len < 0) {
                    ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                    close(sock);
                    sock = -1;
                    break; 
                }

                int reply_len = BuildResponse_CP(rx_buffer.data(), len, DNS_MAX_LEN);
                ESP_LOGI(TAG, "Received %d bytes | DNS reply len: %d", len, reply_len);

                if (reply_len <= 0) {
                    ESP_LOGE(TAG, "Failed to build DNS reply");
                    continue; 
                }

                int err = sendto(sock,
                                response_packet_buffer.data(), reply_len,
                                0,
                                (struct sockaddr*)&source_addr, sizeof(source_addr));
                if (err < 0) {
                    ESP_LOGE(TAG, "sendto failed: errno %d", errno);
                    close(sock);
                    sock = -1;
                    break;
                }
            }

        if (sock >= 0) {
            shutdown(sock, 0);
            close(sock);
        }
    }
}

    static constexpr size_t DNS_MAX_LEN = 256;

    /* State Control */
    bool isRunning = false;
    TaskHandle_t dns_task_handle = nullptr;
};