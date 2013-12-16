## Scope ##

This is an Arduino Reef Aquarium Controller that integrates with the Vera home automation gateway.

## Features ##

The Arduino Sketch manages:

* 2 channels of PWM controlled lighting
* Temperature monitoring of tank and LED heatsinks
* Temp controlled LED cooling fan
* Kalk Mixer (in the day)
* Kalk delivery (at night)

The associated Vera plugin provides integration of the following features into the Vera dashboard:

* Temperature monitoring (4 devices)
* PWM dimmer level monitoring (2 devices)
* Relay control monitoring (4 devices)

## Usage & Limitations ##

Create the master device in Vera. The plugin will automatically create all child devices.

The Arduino sketch is designed to work on the Arduino Yun.

Relay control is not implemented in the Vera plugin, as I did not need it. I didn't want the risk of the Kalk mixer and delivery relays accidentally being turned on at the same time.

## License ##

Copyright 2-13 - Chris Birkinshaw

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/