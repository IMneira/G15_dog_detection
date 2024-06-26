#include "detection_responder.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "main_functions.h"
#include "driver/mcpwm_prelude.h"


static const char *TAG = "servo_test";

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2400
#define SERVO_MAX_DEGREE 180
#define SERVO_MIN_DEGREE 0
#define SERVO_PULSE_GPIO 4
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 20000 // 20000 ticks, 20ms

static inline uint32_t example_angle_to_compare(int angle)
{
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

bool servo_is_ready = false;
mcpwm_cmpr_handle_t comparator = NULL;

int dog_counter = 0;


void RespondToDetection(float no_dog_score) {
    int no_dog_score_int = (no_dog_score) * 100 + 0.5;
    
    if (!servo_is_ready) {
        ESP_LOGI(TAG, "Initializing MCPWM servo control");

        mcpwm_timer_handle_t timer = NULL;
        mcpwm_timer_config_t timer_config;
        timer_config.group_id = 0;
        timer_config.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
        timer_config.resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ;
        timer_config.period_ticks = SERVO_TIMEBASE_PERIOD;
        timer_config.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
        
        ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

        mcpwm_oper_handle_t oper = NULL;
        mcpwm_operator_config_t operator_config = {
            .group_id = 0,
        };
        ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

        mcpwm_comparator_config_t comparator_config = {0};
        comparator_config.flags.update_cmp_on_tez = true;
        ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

        mcpwm_gen_handle_t generator = NULL;
        mcpwm_generator_config_t generator_config = {
            .gen_gpio_num = SERVO_PULSE_GPIO,
        };
        ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

        // Set generator action on timer and compare event
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
            generator,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
            MCPWM_GEN_TIMER_EVENT_ACTION_END()
        ));
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
            generator,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        ));

        ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

        if (dog_counter == 0) {
          ESP_LOGI(TAG, "Setting servo to 0 degrees");
          ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(90)));
        }

        ESP_LOGI(TAG, "Servo initialized");
        servo_is_ready = true;
    }

    MicroPrintf("dog score:%d%%", 100 - no_dog_score_int);

    if (no_dog_score <= 0.1) {
      if (dog_counter == 0) {
        ESP_LOGI(TAG, "Dog detected. Moving servo to 90 degrees.");
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(0)));
        dog_counter = 1;
      }
      else ESP_LOGI(TAG, "Dog detected. Keeping servo in current position.");
    }
    else {
      if (dog_counter == 1) {
        vTaskDelay(7000 / portTICK_PERIOD_MS);       
        ESP_LOGI(TAG, "No dog detected. Moving servo to 0 degrees.");
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(90)));
        dog_counter = 0;
      } 
      else ESP_LOGI(TAG, "No Dog detected. Keeping servo in current position.");
    }
}