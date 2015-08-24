# radeon_bios_decode_HP #

Fork of bcc9's 'radeon_bios_decode' which additionally outputs the hotplug ID of each connector.
Use together with redsock_bios_decoder to dump the full port mapping of Radeon vBIOS.

Usage:
```
./radeon_bios_decode < /path/to/rom/file
```