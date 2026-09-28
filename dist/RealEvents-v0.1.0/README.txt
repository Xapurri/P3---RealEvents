Real Events v0.1.0
==================

Real Events is a public release candidate mod for Patrician III.

It adds dynamic economic events to the game:

- Production crises reduce production in affected cities for a limited time.
- Production booms increase production in affected cities for a limited time.
- City production and private/AI production are both affected.
- Tavern Informers can add rumours about active Real Events.

Requirements
------------

Real Events requires the P3Modding modloader and has been validated with the
GOG release of Patrician III.

The expected modloader layout is:

1. Place p3_modloader.dll and Patrician3_modloader.exe in the Patrician III
   installation folder.
2. Create a mods folder beside them.
3. Copy RealEvents.dll into the mods folder.
4. Start the game with Patrician3_modloader.exe.

Installation
------------

Copy:

    RealEvents.dll

to:

    Patrician III\mods\RealEvents.dll

Then launch:

    Patrician3_modloader.exe

Uninstall
---------

Remove RealEvents.dll from the mods folder.

Save data
---------

Real Events stores its own per-campaign state beside the DLL using files named:

    RealEvents_<hash>.dat

Removing the DLL disables the mod. Removing the matching RealEvents_<hash>.dat
file resets Real Events state for that campaign.

Notes
-----

This package does not include Patrician3.exe, Patrician3_modloader.exe,
p3_modloader.dll, save files, object files, debug symbols, or original game
files.

License
-------

Real Events is distributed under the Apache License, Version 2.0. See LICENSE.
