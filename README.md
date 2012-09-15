WhereDial Arduino Code
======================

John McKerrell 2012

The WhereDial Arduino code is distributed under the terms of the Artistic License 2.0. For more
details, see the full text of the license in the file LICENSE.

This is the Arduino source code that runs on the WhereDial. It is quite simple and performs two main functions, retrieving config from http://mapme.at/ and then retrieving the WhereDial's position from wherever the config tells it to retrieve it.

By default the code will initially connect to something like http://mapme.at/api/wheredial.csv?config=yes&mac=00FF00FF00FF (MAC address needs to be changed for your device) mapme.at will then return something similar to the following:

    mapme.at,80,/api/wheredial.csv?mac=00FF00FF00FF,10

The WhereDial will then request the position from http://mapme.at:80/api/wheredial.csv?mac=00FF00FF00FF every 10 seconds and expect to get a result similar to the following:

    210,38f1e633ea58c6ad7fea714202b2c89fbe099f56

The first value in the CSV is the position in degrees that the WhereDial's dial should be turned to, the second value is a hash for the place that the user is at. If the first value changes then the dial will move to the new position. If the first value stays the same but the second changes then the dial will do a complete rotation to indicate that the user has moved to a different place of the same type (e.g. they've moved from one shop to another).
