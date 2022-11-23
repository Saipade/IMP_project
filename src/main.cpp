#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <ESPmDNS.h>
#include <WiFi.h>

#include "driver/gpio.h"
#include "driver/timer.h"

#include "macros.h"
#include "message.h"

/* handles */
esp_timer_handle_t tick_timer_handle = NULL;
TaskHandle_t client_task_handle = NULL;
TaskHandle_t change_animation_state_task_handle = NULL;
QueueHandle_t animation_change_queue = NULL;

/* current state */
int active_rgb = NONE;
int animation_type = PUMP_ANIMATION;
int animation_speed = MEDIUM_SPEED;

/* server credentials */
const char* ssid = "UPC9344314";
const char* pwd = "Cz5azrfdzM7j";

//const char* ssid = "AndroidAP_2942";
//const char* pwd = "kekwkekw";

WiFiServer server(HTTP_PORT);
WiFiClient client;

/**
 * @brief Tick timer callback function
 * 
 */
void tick(void* arg);


/**
 * @brief Set the up the HTTP (TCP) server object
 * 
 */
void setup_server(void) {
    
    /* Connect to WiFi network */
    WiFi.begin(ssid, pwd);

    /* Wait for the connection */
    printf("Connecting to Wi-Fi %s", ssid);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        printf(".");
    }
    printf("\nSuccessfully connected to %s\nIP: %s\n", ssid, WiFi.localIP().toString().c_str());

    /* Set up mdns responder */
    if (not MDNS.begin(MDNS_DEVICE_NAME)) { 
        printf("Error setting up MDNS responder!\n"); 
        while (true) { delay(1000); }
    }

    printf("mDNS started!\n");
    /* Add tcp service */
    MDNS.addService("http", "tcp", HTTP_PORT);

    

    /* Start server */
    server.begin();
    printf("Server started\n");

}


void set_and_start_tick_timer() {

    esp_timer_create_args_t tick_timer_args = {
        .callback = &tick,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tick-timer",
        .skip_unhandled_events = true
    };

    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer_handle, 500000));  // 0.5 by default

}


/**
 * @brief Switches current rgb led color. 
 * Inactive -> red -> blue -> green -> red -> ...
 * 
 */
void switch_rgb_led_color(void) {

    switch (active_rgb) {
        case RED: {
            gpio_set_level(RGB_LED_RED, false);
            gpio_set_level(RGB_LED_BLUE, true);
            active_rgb = BLUE;
        } break;
        case BLUE: {
            gpio_set_level(RGB_LED_BLUE, false);
            gpio_set_level(RGB_LED_GREEN, true);
            active_rgb = GREEN;
        } break;
        case GREEN: {
            gpio_set_level(RGB_LED_GREEN, false);
            gpio_set_level(RGB_LED_RED, true);
            active_rgb = RED;
        } break;
        case NONE:
        default: {
            gpio_set_level(RGB_LED_RED, true);
            active_rgb = RED;
        }
    }

}


/**
 * @brief Sets the pin responsible for current rgb led color 
 * for a single tick
 * 
 */
void keep_rgb_led_color(void) {

    switch (active_rgb) {
        case RED: {
            gpio_set_level(RGB_LED_RED, true);
        } break;
        case GREEN: {
            gpio_set_level(RGB_LED_GREEN, true);
        } break;
        case BLUE: {
            gpio_set_level(RGB_LED_BLUE, true);
        } break;
        case NONE:
        default: {}
    }

}


/**
 * @brief Unsets all rgb led pins for a single tick
 * 
 */
void unset_rgb_led(void) {
    
    gpio_set_level(RGB_LED_RED, false);
    gpio_set_level(RGB_LED_GREEN, false);
    gpio_set_level(RGB_LED_BLUE, false);

}


void animation_pump(void) {

    static int tick = 0, animation_length = 5;

    gpio_set_level(LEFT_LED, tick>0);
    gpio_set_level(MIDDLE_LED, tick>1);
    gpio_set_level(RIGHT_LED, tick>2);
    if (tick==3) { switch_rgb_led_color(); }
    keep_rgb_led_color();
    tick++;
    tick = tick % animation_length;
}


void animation_worm(void) {

    static int tick = 0, animation_length = 8;
    
    unset_rgb_led();
    gpio_set_level(LEFT_LED, tick==0 or tick==7);
    gpio_set_level(MIDDLE_LED, tick==1 or tick==6);
    gpio_set_level(RIGHT_LED, tick==2 or tick==5);
    if (tick==3) { switch_rgb_led_color(); }
    if (tick==4) { keep_rgb_led_color(); }
    tick++;
    tick = tick % animation_length;

}


void animation_snake(void) {

    static int tick = 0, animation_length = 8;

    unset_rgb_led();
    gpio_set_level(LEFT_LED, tick==0 or tick==1 or tick==7);
    gpio_set_level(MIDDLE_LED, tick==1 or tick==2 or tick==6 or tick==7);
    gpio_set_level(RIGHT_LED, tick==2 or tick==3 or tick==5 or tick==6);
    if (tick==3) { switch_rgb_led_color(); }
    if (tick==4 or tick==5) { keep_rgb_led_color(); }
    tick++;
    tick = tick % animation_length;

}


void animation_wave(void) {

    static int tick = 0, animation_length = 6;

    unset_rgb_led();
    gpio_set_level(LEFT_LED, tick==1 or tick==4);
    gpio_set_level(MIDDLE_LED, tick==0 or tick==5);
    gpio_set_level(RIGHT_LED, tick==0 or tick==5);
    if (tick==1) { switch_rgb_led_color(); }
    if (tick==4) { keep_rgb_led_color(); }
    tick++;
    tick = tick % animation_length;

}


void tick(void* arg) {

    switch (animation_type) {
        case WORM_ANIMATION: {
            animation_worm();
        } break;
        case SNAKE_ANIMATION: {
            animation_snake();
        } break;
        case WAVE_ANIMATION: {
            animation_wave();
        } break;
        case PUMP_ANIMATION:
        default: {
            animation_pump();
        }
    }

}


/**
 * @brief Handles animation speed/type changes, consumes queues
 * 
 * @param args 
 */
void change_animation_state_task(void* args) {

    while (true) {
        change_animation_message_t message;
        if (animation_change_queue != NULL) {
            if (xQueueReceive(animation_change_queue, &message, (TickType_t)100) == pdPASS) {
                String string_message(message.res);
                printf("Message: %s\n", string_message.c_str());
                String response;

                if (string_message == "/") { goto construct_page; }
                else if (string_message == "/favicon.ico") {
                    response = "HTTP/1.1 204 No Content\r\n\r\n";
                    goto print_page;
                }
                /* speed change requests */
                else if (string_message == "/speed_slow") { 
                    animation_speed = SLOW_SPEED;
                    esp_timer_stop(tick_timer_handle);
                    esp_timer_start_periodic(tick_timer_handle, 1000000);
                } else if (string_message == "/speed_medium") { 
                    animation_speed = MEDIUM_SPEED;
                    esp_timer_stop(tick_timer_handle);
                    esp_timer_start_periodic(tick_timer_handle, 500000);
                } else if (string_message == "/speed_high") { 
                    animation_speed = HIGH_SPEED; 
                    esp_timer_stop(tick_timer_handle);
                    esp_timer_start_periodic(tick_timer_handle, 250000);
                /* animation change requests */
                } else if (string_message == "/animation_pump") { 
                    animation_type = PUMP_ANIMATION; 
                } else if (string_message == "/animation_worm") { 
                    animation_type = WORM_ANIMATION; 
                } else if (string_message == "/animation_snake") {
                    animation_type = SNAKE_ANIMATION; 
                } else if (string_message == "/animation_wave") { 
                    animation_type = WAVE_ANIMATION; 
                }
                /* 404 */
                else {
                    printf("Reponse code: 404\n");
                    response = page_404;
                    goto print_page;
                }
construct_page:
                printf("Reponse code: 200\n");
                response += html_start;
                if (animation_speed == SLOW_SPEED) { 
                    response += "<a class=\"button on\" href=\"/speed_slow\">Slow</a>\n";
                } else { 
                    response += "<a class=\"button off\" href=\"/speed_slow\">Slow</a>\n";
                }
                if (animation_speed == MEDIUM_SPEED) { 
                    response += "<a class=\"button on\" href=\"/speed_medium\">Medium</a>\n";
                } else { 
                    response += "<a class=\"button off\" href=\"/speed_medium\">Medium</a>\n";
                }
                response += second_row;
                if (animation_speed == HIGH_SPEED) { 
                    response += "<a class=\"button on\" href=\"/speed_high\">High</a>\n";
                } else { 
                    response += "<a class=\"button off\" href=\"/speed_high\">High</a>\n";
                }
                response += html_animation_buttons;
                if (animation_type == PUMP_ANIMATION) { 
                    response += "<a class=\"button on\" href=\"/animation_pump\">Pump</a>\n"; 
                } else { 
                    response += "<a class=\"button off\" href=\"/animation_pump\">Pump</a>\n"; 
                }
                if (animation_type == WORM_ANIMATION) { 
                    response += "<a class=\"button on\" href=\"/animation_worm\">Worm</a>\n"; 
                } else { 
                    response += "<a class=\"button off\" href=\"/animation_worm\">Worm</a>\n"; 
                }
                response += second_row;
                if (animation_type == SNAKE_ANIMATION) { 
                    response += "<a class=\"button on\" href=\"/animation_snake\">Snake</a>\n"; 
                } else { 
                    response += "<a class=\"button off\" href=\"/animation_snake\">Snake</a>\n"; 
                }
                if (animation_type == WAVE_ANIMATION) { 
                    response += "<a class=\"button on\" href=\"/animation_wave\">Wave</a>\n"; 
                } else { 
                    response += "<a class=\"button off\" href=\"/animation_wave\">Wave</a>\n"; 
                }
                response += html_end;
print_page:
                client.write(response.c_str());
                delay(1);
            }
        }
    }

}


/**
 * @brief Handles client connection, produces queues
 * 
 * @param args 
 */
void task_client(void* args) {
    
    while (true) {

        client = server.available();
        if (not client) { continue; }

        /* Wait for data from client to become available */
        while (client.connected() and not client.available()) { delay(1); }

        /* parse request */
        String request = client.readStringUntil('\r');
        int address_start = request.indexOf(' ');
        int address_end = request.indexOf(' ', address_start + 1);
        if (address_start == -1 || address_end == -1) { continue; }
        request = request.substring(address_start + 1, address_end);
        const char* _request = request.c_str();
        printf("Request: %s\n", _request);

        /* create queue */
        animation_change_queue = xQueueCreate(1, sizeof(change_animation_message_t));

        /* copy request to message structure */
        change_animation_message_t message;
        memset(&(message.res), 0, 20);
        strncpy(message.res, _request, strlen(_request));
        
        /* send queue */
        while (xQueueSend(animation_change_queue, (void *)&message, (TickType_t)100) != pdTRUE) {;}
        
        /* give the web browser time to receive the data */
        delay(1);
        
        /* close the connection */
        client.stop();

    }

}


extern "C" void app_main(void) {
    
    /* initialise arduino */
    initArduino();

    /* setup server */
    setup_server();

    /* initialise pins */
    gpio_pad_select_gpio(RGB_LED_RED);
    gpio_pad_select_gpio(RGB_LED_BLUE);
    gpio_pad_select_gpio(RGB_LED_GREEN);
    gpio_pad_select_gpio(RIGHT_LED);
    gpio_pad_select_gpio(MIDDLE_LED);
    gpio_pad_select_gpio(LEFT_LED);

    /* initialise output pins  */
    gpio_set_direction(RGB_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_LED_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RIGHT_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(MIDDLE_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEFT_LED, GPIO_MODE_OUTPUT);

    /* initialise timer */
    set_and_start_tick_timer();

    /* create tasks */
    xTaskCreate(task_client, "client", STACK_SIZE, NULL, 1, &client_task_handle);
    xTaskCreate(change_animation_state_task, "change the animation state", STACK_SIZE, NULL, 0, &change_animation_state_task_handle);
    //xTaskCreatePinnedToCore(task_client, "client", STACK_SIZE, NULL, 0, &client_task_handle, 0);
    //xTaskCreatePinnedToCore(change_animation_state_task, "change the animation state", STACK_SIZE, NULL, 1, &change_animation_state_task_handle, 1);

}
