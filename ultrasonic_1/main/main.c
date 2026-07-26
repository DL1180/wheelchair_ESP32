#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#define TRIG_GPIO GPIO_NUM_9
#define ECHO_GPIO GPIO_NUM_8

static float measure_distance_cm(void)
{
    //10 us trigger pulse
    gpio_set_level(TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_GPIO, 0);

    //Wait-->echo HIGH
    int64_t start_wait = esp_timer_get_time();
    while (gpio_get_level(ECHO_GPIO) == 0) {
        if (esp_timer_get_time()-start_wait > 30000) {
            return -1.0; //timeout
        }
    }

    // Measure time echo stays HIGH
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_GPIO) == 1) {
        if (esp_timer_get_time()-start_time > 30000) {
            return -1.0; //timeout
        }
    }
    int64_t pulse_duration_us = esp_timer_get_time() - start_time;
    //Speed of sound: 343 m/s = 0.0343 cm/us
    //Divide by 2: sound travels to object and back
    float distance_cm = (pulse_duration_us*0.0343)/ 2.0;
    return distance_cm;
}

void app_main(void)
{
    gpio_config_t trig_config = {
        .pin_bit_mask = (1ULL << TRIG_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&trig_config);
    gpio_set_level(TRIG_GPIO, 0);
    gpio_config_t echo_config = {
        .pin_bit_mask = (1ULL << ECHO_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&echo_config);

    printf("HC-SR04 ultrasonic test started\n");

    while (1) {
        float distance = measure_distance_cm();
        if (distance < 0) {
            printf("No echo received\n");
        } else {
            printf("Distance: %.2f cm\n", distance);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
