# Build

```
$ get_idf
$ idf.py build
$ idf.py -p /dev/ttyUSB0 flash
```

After fetch from github do:
 - ```idf.py fullclean```
 - ```idf.py build```
 - ```idf.py -p /dev/ttyUSB0 flash```

## Serial monitor
Pins 1 and 3 are the default uart0 pins that are used for the serial monitor. Use different pins.

 Use minicom to monitor uart
 - ```minicom -D /dev/ttyUSB0```

 or
 https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-monitor.html

 - ```idf.py -p /dev/ttyUSB0 monitor``` : use Ctrl-] to close it.
