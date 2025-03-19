# peripheral_lbs_ext
This is based on the youtube clip from Nordic Semiconductor:
https://www.youtube.com/watch?v=CY0Vq_Lgs4k&list=PLx_tBuQ_KSqG-leiJDfOKf_ZmCqHnbi4r
,but BME280 was used instead of BMI270 and BME688.
for overall information, you can refer the attached nRF Connect SDK Hands-on.pdf.

Environment:
-. based on NCS 2.9.0.
-. verified on nRF54L15DK.

-. Connection with BME280:
-----------------------------------------------
    BME280  -   nRF54L15DK
-----------------------------------------------    
    SDO     -   GND
    CSB     -   VDDIO
    SDA     -   P1.12
    SCL     -   P1.11
    GND     -   GND
    VCC     -   VDDIO
-----------------------------------------------

-.Revision history.
Tag1 : sense the actual data from the sensor, BME380. If you need simulated data without actual sensor, 
enable CONFIG_SIMULATED_SENSOR in the prj.conf.
