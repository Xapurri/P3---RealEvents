# Third-party research and attribution

Real Events is an independent mod built on top of community reverse-engineering knowledge.

## P3Modding

Public references used during development include:

- Patrician 3 Insights  
  https://p3modding.github.io/

- Modloader documentation  
  https://p3modding.github.io/modloader.html

- `p3_ida_scripts`  
  https://github.com/P3Modding/p3_ida_scripts

- `p3-lib`  
  https://github.com/P3Modding/p3-lib

The P3Modding `p3-lib` repository is distributed under the Apache-2.0 license. Real Events does not claim authorship of upstream P3Modding discoveries.

## Community Cheat Engine research

Existing Cheat Engine material was used as a reference for several IDs and offsets, including raw town IDs, ware IDs, facility type IDs, and the Merchant hometown field.

Before the public release is uploaded, the exact Cheat Engine table/source and author should be identified and credited here if available.

## Real Events runtime discoveries

The following items were specifically traced and runtime-tested while developing Real Events:

- Town production dispatcher behavior when temporarily scaling facility efficiency.
- Restoration paths through the three town dispatcher exits.
- Private/Trader production dispatcher behavior for player and AI facilities.
- Combined Town + Private production scaling.
- Final Tavern Informer text pointer at the hook point used by the mod.
- Safe append strategy using a separate text buffer rather than modifying the vanilla string.
