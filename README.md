## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

https://esp-idf-lib.readthedocs.io/en/latest/groups/bmp180.html

https://github.com/espressif/esp-iot-solution

# esp32c3_supermini
    Pin     | ADC | func |
GPIO_NUM_0  | A0  |      |photoresistor
GPIO_NUM_1  | A1  |      |
GPIO_NUM_2  | A2  |      |
GPIO_NUM_3  | A3  |      |
GPIO_NUM_4  | A4  | SCK  | max7219 CLK
3V3
GND
5V
-------------------------------------
GPIO_NUM_5  | A5  | MISO | max7219 CS
GPIO_NUM_6  |     | MOSI | max7219 DIN
GPIO_NUM_7  |     | SS   | 
GPIO_NUM_8  |     | SDA  | LED           |HTU2x SDA
GPIO_NUM_9  |     | SDL  | BOOT_BUTTON   |HTU2x SDL
GPIO_NUM_10 |     |      |
GPIO_NUM_20 |     | RX   |
GPIO_NUM_21 |     | TX   |

#Display MAX7219 


https://github.com/DiegoPaezA/ESP32-freeRTOS/blob/master/i2c_scanner/main/i2c_scanner.c
https://github.com/espressif/esp-iot-solution/tree/dd15770d0f8c5fedea78809737f04b63d3765c83/examples/sensors/sensor_hub_monitor


. /home/oleksandr/ides/tools/esp-idf-v5.3/export.sh
export IOT_SOLUTION_PATH=/home/oleksandr/personal/repo/esp-iot-solution
idf.py set-target esp32c3
idf.py build
idf.py menuconfig

null in the env https://github.com/espressif/idf-eclipse-plugin/issues/535https://components.espressif.com/

Check the values in your sdkconfig for LOG_MAXIMUM_LEVEL. It defaults to matching LOG_DEFAULT_LEVEL which limits what values you can use by default.


[mqtt test]
sudo apt-get install mosquitto mosquitto-clients

sudo vim /etc/mosquitto/mosquitto.conf 
add
  listener 1883 0.0.0.0
  allow_anonymous true

sudo ufw allow 1883

sudo /etc/init.d/mosquitto restart

one terminal
mosquitto_sub -d -t hello/world
mosquitto_sub -d -t stat/weather -h central.local
Connect to another ssh session and run
mosquitto_pub -d -t hello/world -m "Hello from terminal window 2!"
mosquitto_pub -h central.local -d -t stat/pr_clock -m "Hello from terminal window 2!"
sudo tail -f /var/log/mosquitto/mosquitto.log

mosquitto_sub -d -t '#'
https://github.com/nopnop2002/esp-idf-json/tree/master/json-basic-object

[rtc]
CONFIG_RTC_CLK_SRC ->External 32kHz crystal
[sntp]
Request interval to update time
[button]
todo boot btn to go provision mode?

[termomenter]
https://www.lucadentella.it/en/2017/10/13/esp32-24-i2c-un-esempio-pratico-con-sensore-htu21d/
https://github.com/kimsniper/htu21d/tree/master/examples/esp32_implementation/main
si7021 (htu2x)

[esp-idf-lib]
https://esp-idf-lib.readthedocs.io/en/latest/index.html
git clone git@github.com:UncleRus/esp-idf-lib.git

[adc]
https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32/api-reference/peripherals/adc.html
#photoresistor (GND ) -photoresistor- (A0) -resistor 10k- (+3.3)

https://esp32tutorials.com/esp32-esp-idf-max7219-dot-matrix-display

https://xantorohara.github.io/led-matrix-editor/#7e1818181c181800|7e060c3060663c00|3c66603860663c00|30307e3234383000|3c6660603e067e00|3c66663e06663c00|1818183030667e00|3c66663c66663c00|3c66607c66663c00|3c66666e76663c00

[json]
https://github.com/nopnop2002/esp-idf-json

[cmd list]
mosquitto_pub -d -t cmd/64E833885558 -m '{"cmd":"ldr","payload":{"max":1530,"min":4095}}'
mosquitto_pub -d -t cmd/64E833885558 -m '{"cmd":"restart"}'
mosquitto_pub -d -t cmd/64E833885558 -m '{"cmd":"brightness","payload":{"max":8,"min":0}}'

{"cmd":"display","payload":{"segment_rotation":0,"segment_upsidedown":false,"mirrored":false}}

'{"cmd":"display","payload":{"segment_rotation":3,"segment_upsidedown":true,"mirrored":true}}

'{"cmd":"timezone","payload":{"tz":"GMT-3"}}'







