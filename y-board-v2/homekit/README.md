# HomeKit Demo

This demo makes the Y-Board v2 a HomeKit controllable device. It presents the LEDs as an RGB light bulb, button one as a way to turn on and off the LEDs (and notify HomeKit of the change), and buttons two and three as programmable switches that you can program from within the Home app. The knob, switches, and buzzer are unused.

There are fancier ways of connecting the Y-Board to HomeKit, but this demo provides the simplest way. You must plug in the Y-Board to a computer, open a serial connection, and then type "W" to connect to WiFi. The [CLI documentation](https://github.com/HomeSpan/HomeSpan/blob/master/docs/CLI.md) provides more details, and the [paragraph on pairing](https://github.com/HomeSpan/HomeSpan/blob/463bdd4cdaaa7cf32cce0a71b3a2fd88e508cd66/docs/UserGuide.md#pairing-to-homekit) is also important to read. 


