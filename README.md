# diyDAQ
An Arduino data acquisition box for pressure and load testing amateur rocket motors


Overview

Once you decide to delve into making experimental rocket motors, big or small, you will need a way to test those motors, before putting them in rockets. About three years ago, my son and I built the first version of this DAQ to support load testing of small 29mm KNSB motors for his science project. We have since improved it and scaled it up to support pressure and load testing on our 75mm motors, but it can really be used for any (amateur) motor large or small. We also added temperature probes to the testing, but removed it in this version, as our probes had too much latency (researching IR probes next). 

This DAQ uses a small inexpensive LCD screen, so that results and graphs are immediately available in the field, and detailed raw data is available on an SD card for review on a computer. It has a friendly interface to monitor, sample, or review past tests on the box. 

You should have a basic knowledge of how to code and load libraries in Arduino and how to solder/connect small components. The code is a hack and Arduino provides a lot of latitude for things like sloppy string handling, but it is battle tested and the code should be easy to follow. It is a good starting point for schools or student groups that want to build their own DAQ on the cheap.  

This version samples at a rate of about 85 Hz. It is limited by the Load Test amplifier, so if you only use pressure you can get much faster sampling. We’ve found that 85 samples a second is fine for our needs and it gives us a very accurate profile of the burn with very small anomalies and events showing up in the raw data. You can choose to only test pressure, load, or both. One or the other is usually enough to characterize the motor, but having both side-by-side gives you a lot more information about your motor. 

Functionality

The DAQ has three modes set by a toggle switch:  1) monitor mode 2) sample mode 3) review mode.  

In monitor mode it will just monitor the load cell and the pressure transducer in real-time and provide the weight and pressure readings. You can also tare the load cell (press select) or display the config file off the SD card (press scroll) in this mode.

In Sample mode, you start sampling by pressing the select button. This will put it into active sampling. In this mode it will:  a) capture a “pre-sample” for three seconds b) main sample (up to ten seconds) and c) post sample ten seconds. Each of these is configurable. In pre-sample it is keeping a rolling three seconds of data while it monitors the load cell and pressure sensor to come up to threshold. After either one crosses the starting threshold it will go into “main sampling”, while watching for both pressure and load to drop below the stopping threshold. The main sampling will timeout after ten seconds (configurable) and has a minimum burn time set (2 seconds configurable). After the main event it will post sample for another ten seconds, just in case we had a false start or chuffing. So far, we haven’t needed to fall back on the pre data or post data, but good to know it is there. At the conclusion of sampling, it will display burn time, max pressure, avg pressure, max load, total impulse, avg load, motor common name, and more. While it is actively sampling a large LED will change colors between Green (pre sample, Red (main sample), Blue (post sample), and White (all done). This allows you to visually see what state it is in from a distance.

Switching to Review Mode, allows you to scroll through the last five tests to see summary data, a pressure graph, and a load graph. While in Review mode, the second toggle switch allows you to move between summary, pressure graph, and load graph. 

All of the settings for the DAQ are stored on a file (config.txt) on the SD card. This allows for easy changes without having to open up and recompile Arduino code. This includes turning on/off load or pressure, calibration settings for the pressure sensor and load cell, as well as all the timings and thresholds. 

Equipment and Libraries

We chose to build this using an Arduino Mega, as it is a 5v board and most of the analog pressure transducers operate 0-4.5v DC. The Mega is also really flexible with input voltage and has plenty of CPU to drive the TFT and sensors. 

For the display we are using an Adafruit 3.5" TFT touchscreen HXD8357D 320x480 with SD card. We are not actually using the touchscreen features, but this is a good quality inexpensive display that doesn’t require any additional hardware. It has basic graphing capability, colors and fonts, and is easy to code. It also has a built-in SD card, since both use the Arduino SPI bus that is convenient. Library: https://learn.adafruit.com/adafruit-3-5-color-320x480-tft-touchscreen-breakout/adafruit-gfx-library 

For the load cell we have a range of strain gauge cells that connect to an HX711 amplifier. Our smallest cell is 100Kg and largest 1000Kg. Each of these need their own simple calibration and the HX711 needs its own library:  https://github.com/bogde/HX711 

For the pressure sensor, we are just using a cheap three wire 1/8” NPT sensor from eBay that has an analog range of 0-4.5v. We are currently using a 1600psi sensor, but again you can get them in just about any value. These will also need a calibration factor. For our sensor, the base analog value is 100 (at 0.0v) and 50 for each 100psi. We use a grease gun with an analog pressure gauge to verify/calibrate the pressure transducer. It is an easy way to simulate a range of psi values up to 2000+ psi. When testing big motors we put a little grease in the hole, before screwing in the sensor, to protect it from the heat. Surprisingly, these little sensors do just fine under the heat and stress of the motor.

The other components are all pretty standard switches, push buttons, or LEDs. We use a 7.4v Lipo battery, but you could use a few standard 9v batteries in parallel (we did that on our first build). We added a volt meter read-out to our box to monitor the battery voltage. 

We extend the sensors 30 feet from the box using Cat6 ethernet cable. The pressure sensor runs the whole length back to the Arduino analog input, but the HX711 for the load sensor is on the “business end” of the 30 foot cable to minimize noise during testing. If you are testing smaller motors you don’t need that much cable. On earlier versions we used RJ45 connectors on each end for the cable, but we found that during large tests they jiggled loose, so no we cut the cable and soldered on both ends. 


Shopping List

Arduino Mega ($38):  
https://www.amazon.com/dp/B0046AMGW0/ref=cm_sw_em_r_mt_dp_JXTvFb2R2KDRK

Arduino Mega shield($12):
https://www.amazon.com/dp/B00UYO4VWA/ref=cm_sw_em_r_mt_dp_yYTvFbYCPX2B9

Adafruit 3.5" TFT touchscreen HXD8357D 320x480 ($40 digi-key)
https://www.adafruit.com/product/2050 

HX711 amplifier ($6):
https://www.amazon.com/dp/B010FG9RXO/ref=cm_sw_em_r_mt_dp_IITvFbDPTNS30

(2) SPDT three way toggle switches ($12):
https://www.amazon.com/dp/B07PDQN6P8/ref=cm_sw_em_r_mt_dp_WDTvFbDR72Z4J

(2) Momentary push button switches ($8)
https://www.amazon.com/dp/B01G821G92/ref=cm_sw_em_r_mt_dp_MHTvFbE78DQX2

Three color large LED ($1)
https://www.amazon.com/dp/B01CI6EWHK/ref=cm_sw_em_r_mt_dp_v0TvFb9VB34HS

(3) 330 ohm resistors ($3): 
https://www.amazon.com/dp/B072L4B7W7/ref=cm_sw_em_r_mt_dp_a2TvFbS55HZT7

Battery 7.4v Lipo($20): 
https://www.amazon.com/dp/B07NRM7HFZ/ref=cm_sw_em_r_mt_dp_X2TvFbKQR6PXH

Battery switch ($4): 
https://www.amazon.com/dp/B008UTX208/ref=cm_sw_em_r_mt_dp_H3TvFb511478D

Pressure Transducer ($20): 
https://www.ebay.com/itm/UNIVERSAL-5V-PRESSURE-TRANSDUCER-SENDER-1600-PSI-OIL-FUEL-AIR-WATER-W-CONNECTOR/223214955490

Load Cell ($30): 
https://www.amazon.com/dp/B07RL2D2JQ/ref=cm_sw_em_r_mt_dp_WeUvFb94D6PFH

White ammo box ($10):
https://www.amazon.com/dp/B07M5VB1G5/ref=cm_sw_em_r_mt_dp_pbUvFbMNRBB08

30 ft of Cat6 cable ($16): 
https://www.amazon.com/dp/B07KX26QWJ/ref=cm_sw_em_r_mt_dp_QfUvFbZE87JDV

Battery volt display ($9.00): 
https://www.amazon.com/dp/B078LVLHNF/ref=cm_sw_em_r_mt_dp_LiUvFbRPHSXNG

Note:  This is about $200+ in supplies, but if you get generics or lower cost components you can probably cut the price in half. 

Here is the Grease gun info for calibrating/testing the pressure guage (McMaster-Carr Order):

1060K52	Compact Pistol-Grip Grease Gun for Standard Fitting, 6" Long Nozzle (21.53)

4089K64	Single Scale Pressure Gauge with Plastic Case, 1/4 NPT Male ($12.17)

4112T23	Compact High-Pressure Brass Ball Valve with Lever Handle ($12.18)

45525K552	High-Pressure 304 Stainless Steel Pipe Fitting, Cross Connector ($28.50)

4443K722	High-Pressure 316 Stainless Steel Pipe Fitting, Plug with External Hex Drive 50785K61	High-Pressure Brass Pipe Fitting, Bushing Adapter with Hex Body, 1/4 Male x 1/8 

48805K571	Precision Extreme-Pressure 316 Stainless Steel Fitting, Right-Angle Tee Adapter, 


SD Card Files

preXX.txt – presample raw data (milliseconds, analog pin pressure value, load cell value (lbs)) 

MainXX.txt – main sample raw data (milliseconds, analog pin pressure value, load cell value (lbs))

PostXX.txt – post sample raw data (milliseconds, analog pin pressure value, load cell value (lbs))

SumXX.txt – summary of the test results in plain text

Config.txt (values must be on exact line numbers with <cr> at end) – note you can call the function CreateConfigFile() in the Arduino code to write a new file from scratch. If no file exists it will use the defaults in the global declarations. 

File Format:
 * Pressure enabled (1 or Y)
 * Load enabled (1 or Y)
 * Sample interval (ms)
 * Pressure sensor base value (integer)
 * Pressure sensor 100 PSI value (integer)
 * Pressure value to start (PSI)
 * Pressure value to end (PSI)
 * Load cell calibration value (set to 0 then divide by sample weight)
 * Load value to start sampling (in lbs or units)
 * Load value to end sampling (in lbs or units)
 * Main Sample loop timeout (ms - default to 10 seconds 10000)
 * Main sample loop minimum time (ms - default to 2 seconds 2000)
 * Post sample duration (ms - default to 10 seconds 10000)
