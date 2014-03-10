arduino-build-monitor
=====================

Simple arduino based device that goes off when a build breaks. Should be able to work with any system (jenkins, travis etc).

### Hardware

1x RGB LED (for status dispaly / errors)
1x Transistor (to power the siren / strobe)
1x Sparkfun pro micro (or similar) https://www.sparkfun.com/products/12587
1x ENC28J60 Ethernet module (ebay: http://tinyurl.com/nfromrz)
1x Strobe light (http://www.maplin.co.uk/p/mini-led-police-beacon-n74kf)

### Requirements

This requires the Eth lib to work from https://github.com/jcw/ethercard, see the docs for installation

### Usage

To break a build simply call `<ip>/?build=fail` which will set off the device

When the build is good call `<ip>/?build=pass` which clears any alerts

### Roadmap

- Configure how long the alarm goes off
- Sound / siren
- fetch build status instead of waiting for it to come
- power management
- PoE
- WiFi?
