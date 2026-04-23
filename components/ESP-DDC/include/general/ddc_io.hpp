
/* Includes */
#include "soc/gpio_num.h"
#include <cassert>
#include <esp_err.h>
#include <driver/gpio.h>

/* Class */
class Pin
{
    public:
    /* Constructor */
    Pin(gpio_num_t pin_num, gpio_mode_t mode)
    : pin_num(pin_num)
    {
        esp_err_t ret = gpio_reset_pin(pin_num);
        assert(ret == ESP_OK);
        ret = gpio_set_direction(pin_num, mode);
        assert(ret == ESP_OK);
    }
    ~Pin()
    {
        gpio_reset_pin(pin_num);
    }

    enum class state : uint8_t
    {
        low = 0,
        high = 1
    };

    esp_err_t set2(state s)
    {
        return gpio_set_level(pin_num, static_cast<uint8_t>(s));
    }

    esp_err_t toggle()
    {
        return gpio_set_level(pin_num, !gpio_get_level(pin_num));
    }

    protected:
    private:
    gpio_num_t pin_num;

};