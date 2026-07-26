#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SENSOR1_TRIG_GPIO GPIO_NUM_1
#define SENSOR1_ECHO_GPIO GPIO_NUM_2

#define SENSOR2_TRIG_GPIO GPIO_NUM_9
#define SENSOR2_ECHO_GPIO GPIO_NUM_8

#define LED_GPIO GPIO_NUM_5

#define STOP_DISTANCE_CM 15.0f
#define CLEAR_DISTANCE_CM 20.0f
#define ECHO_TIMEOUT_US 25000
#define SENSOR_DELAY_MS 50

static const char *TAG = "ULTRASONIC";
static bool stop_active = false;

static float measure_distance_cm(gpio_num_t trig, gpio_num_t echo)
{
    gpio_set_level(trig, 0);
    esp_rom_delay_us(2);

    gpio_set_level(trig, 1);
    esp_rom_delay_us(10);
    gpio_set_level(trig, 0);

    int64_t wait_start = esp_timer_get_time();

    while (gpio_get_level(echo) == 0) {
        if (esp_timer_get_time() - wait_start > ECHO_TIMEOUT_US) {
            return -1.0f;
        }
    }

    int64_t echo_start = esp_timer_get_time();

    while (gpio_get_level(echo) == 1) {
        if (esp_timer_get_time() - echo_start > ECHO_TIMEOUT_US) {
            return -1.0f;
        }
    }

    int64_t echo_end = esp_timer_get_time();
    int64_t echo_duration = echo_end - echo_start;

    return echo_duration * 0.0343f / 2.0f;
}

static bool obstacle_close(float distance)
{
    return distance > 0.0f && distance <= STOP_DISTANCE_CM;
}

static bool sensor_clear(float distance)
{
    return distance < 0.0f || distance > CLEAR_DISTANCE_CM;
}

static void activate_stop(const char *sensor_name,
                          float distance,
                          int64_t latency_us)
{
    stop_active = true;
    gpio_set_level(LED_GPIO, 1);

    ESP_LOGW(TAG, "EMERGENCY STOP");
    ESP_LOGW(TAG, "Sensor: %s", sensor_name);
    ESP_LOGW(TAG, "Distance: %.1f cm", distance);
    ESP_LOGW(TAG, "Trigger-to-decision latency: %.3f ms",
             latency_us / 1000.0);
}

void app_main(void)
{
    gpio_config_t output_config = {
        .pin_bit_mask =
            (1ULL << SENSOR1_TRIG_GPIO) |
            (1ULL << SENSOR2_TRIG_GPIO) |
            (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config_t input_config = {
        .pin_bit_mask =
            (1ULL << SENSOR1_ECHO_GPIO) |
            (1ULL << SENSOR2_ECHO_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&output_config);
    gpio_config(&input_config);

    gpio_set_level(SENSOR1_TRIG_GPIO, 0);
    gpio_set_level(SENSOR2_TRIG_GPIO, 0);
    gpio_set_level(LED_GPIO, 0);

    ESP_LOGI(TAG, "Two-sensor collision-prevention test started");

    while (true) {
        int64_t start1 = esp_timer_get_time();

        float distance1 = measure_distance_cm(
            SENSOR1_TRIG_GPIO,
            SENSOR1_ECHO_GPIO
        );

        int64_t latency1 = esp_timer_get_time() - start1;
ESP_LOGI(TAG, "Sensor 1 latency: %.3f ms",
         latency1 / 1000.0);
        if (distance1 > 0.0f) {
            ESP_LOGI(TAG, "Sensor 1: %.1f cm", distance1);
        } else {
            ESP_LOGW(TAG, "Sensor 1: no valid echo");
        }

        if (!stop_active && obstacle_close(distance1)) {
            activate_stop("Sensor 1", distance1, latency1);
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_DELAY_MS));

        int64_t start2 = esp_timer_get_time();

        float distance2 = measure_distance_cm(
            SENSOR2_TRIG_GPIO,
            SENSOR2_ECHO_GPIO
        );

        int64_t latency2 = esp_timer_get_time() - start2;
ESP_LOGI(TAG, "Sensor 2 latency: %.3f ms",
         latency2 / 1000.0);

        if (distance2 > 0.0f) {
            ESP_LOGI(TAG, "Sensor 2: %.1f cm", distance2);
        } else {
            ESP_LOGW(TAG, "Sensor 2: no valid echo");
        }

        if (!stop_active && obstacle_close(distance2)) {
            activate_stop("Sensor 2", distance2, latency2);
        }

        if (stop_active &&
            sensor_clear(distance1) &&
            sensor_clear(distance2)) {

            stop_active = false;
            gpio_set_level(LED_GPIO, 0);
            ESP_LOGI(TAG, "Obstacle cleared");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_DELAY_MS));
    }
}
