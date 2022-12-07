/* stack size */
#define STACK_SIZE 2048

/* ports */
#define HTTP_PORT 80

/* pin macros */
#define RGB_LED_RED     GPIO_NUM_14
#define RGB_LED_BLUE    GPIO_NUM_27
#define RGB_LED_GREEN   GPIO_NUM_16
#define RIGHT_LED       GPIO_NUM_17
#define MIDDLE_LED      GPIO_NUM_25
#define LEFT_LED        GPIO_NUM_26

/* currently active color on rgb led */
enum rgb_colors { NONE, RED, BLUE, GREEN };

/* animation aliases */
enum animations { PUMP_ANIMATION, WORM_ANIMATION, SNAKE_ANIMATION, WAVE_ANIMATION };

/* animation speed aliases */
enum speed { SLOW_SPEED, MEDIUM_SPEED, HIGH_SPEED };

/* mdns macros */
#define MDNS_DEVICE_NAME "esp32-led-controller"


char* html_start = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" \
"<!DOCTYPE html>" \
"<html>\n" \
"    <head>\n" \
"        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n" \
"        <link rel=\"icon\" href=\"data:;base64,=\">" \
"        <title>ESP32 LED control</title>\n" \
"        <style>\n" \
"            html { \n" \
"                font-family: Helvetica; \n" \
"                margin: 0px 0px; \n" \
"                width: 100vw;\n" \
"                height: 100vh;\n" \
"                overflow: auto;" \
"            }\n" \
"            body { \n" \
"                margin-top: 50px;\n" \
"            } \n" \
"            h2 { \n" \
"                margin: 50px auto 30px;\n" \
"            } \n" \
"            h4 {\n" \
"                margin-bottom: 50px;\n" \
"            }\n" \
"            p {\n" \
"                font-size: 14px;\n" \
"            }\n" \
"            .buttons {\n" \
"                display: flex; \n" \
"                width: 70vh; \n" \
"            }\n" \
"            .button {\n" \
"                display: block;\n" \
"                width: 70px;\n" \
"                border: none;\n" \
"                color: white;\n" \
"                padding: 10px 20px;\n" \
"                text-decoration: none;\n" \
"                font-size: 20px;\n" \
"                margin: 0px 20px 35px;\n" \
"                cursor: pointer;\n" \
"                border-radius: 4px;\n" \
"            }\n" \
"            .off { \n" \
"                background-color: #3498db;\n" \
"            }\n" \
"            .on { \n" \
"                background-color: blue;\n" \
"            }\n" \
"        </style>\n" \
"    </head>\n" \
"    <body>\n" \
"        <div class=\"content\">\n" \
"            <h2>ESP32 Web Server</h1>\n" \
"            <h4>Change the animation speed</h2>\n" \
"            <div class=\"buttons\">\n";

char* html_animation_buttons = "</div>\n<h2>Change the animation</h2>\n" \
"               <div class=\"buttons\">\n";


char* second_row = "</div>\n" \
"               <div class=\"buttons\">\n";


char* html_end = "</div>\n" \
"        </div>\n" \
"    </body>\n" \
"</html>\r\n\r\n";

char* page_404 = "HTTP/1.1 404 Not Found\r\n\r\n";
