Custom ethertype wrapping our discovery protocol: 0xDEAD

All discovery protocol packets are broadcasted, we're managing L2 here

Packet format:
   ethernet:
      6x DST MAC
      6x SRC MAC
      2x Ethertype (0xDEAD)
   discovery protocol:
      1x Version (0x00)
      1x Segment

Rough idea:
   peer broadcasts N packets each segment, segment tick = commit
   speeds up broken link detection:
      if we receive only the SRC on a single interface, we know the MAC is unreachable on other interfaces within 2 ticks
