% ${NAME}(1) | General Commands Manual

NAME
====

${NAME} - Poor man's link aggregator

SYNOPSIS
========

`${NAME} [options]`

DESCRIPTION
===========

${NAME} is a tool for bonding network interfaces together when the hardware on
the other side of the cable(s) doesn't necessarily support it.

OPTIONS
=======

#### `-h, --help`

Show the help message and exit

#### `-c, --config <PATH>`

Load a custom configuration instead of the default (/etc/${NAME}.conf)

CONFIGURATION
=============

The configuration is an ini-like format, where the sections are the name of the
device being configured.

The configuration can be reloaded by sending the USR1 signal to the process.

#### broadcast

Select the broadcast method for the bonded interface

- flood: send out broadcast packages out over all interfaces
- balanced: send out broadcast packages according to balancing strategy

#### mode

Select the balancing mode for non-broadcast packages

- active-backup: define a primary and backup interface
- broadcast: always sent packets to all interfaces
- balance-rr: weighted round-robin balancing

#### mac

Define the mac address of the bonded interface

#### master

Define which bonded interface the current described interface belongs to

#### weight

Behaves differently depending on the mode the bonded interface is in:

- active-backup: priority of the interface (higher = more preferred)
- broadcast: no effect
- balance-rr: share of traffic the interface transmits

Try to keep this number as low as possible, as it's used directly to define
the number of packets to send before moving to the next interface in the
round-robin strategy.

### Example:

```ini
[bond0]
broadcast=flood
mode=balance-rr
mac=01:23:45:67:89:AB

[eth0]
master=bond0
weight=1

[eth1]
master=bond0
weight=10
```

LICENSE
=======

The MIT License (MIT)

Copyright © 2022 finwo

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
