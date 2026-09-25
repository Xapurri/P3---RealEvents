# Memory layout reference

Addresses below refer to the currently tested Patrician III executable. They should not be assumed portable to other executable builds without validation.

## GameWorld

```text
GAME_WORLD = 0x006DE4A0
```

Known offsets:

| Offset | Meaning | Status |
|---:|---|---|
| `+0x00` | Day of month | Upstream |
| `+0x01` | Month | Upstream |
| `+0x02` | Year | Upstream |
| `+0x04` | Day of year | Upstream |
| `+0x10` | Town count (`word`) | Upstream |
| `+0x14` | Gametime/timestamp | Upstream + runtime confirmed |
| `+0x68` | Towns pointer | Upstream |
| `+0x70` | MerchantFacilities pointer | Upstream |
| `+0x78` | Merchants pointer | Upstream |

Other constants:

```text
Town size = 0x9F8
Town names table = 0x006DDA00
```

## Town

Known fields:

```text
Town + 0x2C0 = stored town index
Town + 0x2C1 = raw town ID
```

## Merchant

```text
Merchant size = 0x650
Player merchant index = 0x24
Player merchant byte offset = 0xE340
Merchants pointer address = 0x006DE518
```

Player money is reached through:

```text
*[0x006DE518] + 0xE340
```

Community Cheat Engine material also identifies Merchant `+0x19` as hometown index; this has not been independently traced in the same way as the production hooks.

## Facility / MerchantFacility leading layout

```cpp
#pragma pack(push, 1)
struct FacilityLayout
{
    uint32_t efficiency;     // +0x00
    uint16_t employees;      // +0x04
    uint8_t  facilityType;   // +0x06
    uint8_t  townIndex;      // +0x07
    uint16_t field08;        // +0x08
    uint16_t field0A;        // +0x0A
    uint16_t employeesMax;   // +0x0C
    uint16_t field0E;        // +0x0E
    uint32_t field10;        // +0x10
};
#pragma pack(pop)
```

MerchantFacility entry size observed/documented: `0x14`.

## Raw town IDs

| ID | Town | ID | Town |
|---:|---|---:|---|
| `00` | Edinburgh | `14` | Aalborg |
| `01` | Newcastle | `15` | Goeteborg |
| `02` | Scarborough | `16` | Naestved |
| `03` | Boston | `17` | Malmoe |
| `04` | London | `18` | Ahus |
| `05` | Bruges | `19` | Stockholm |
| `06` | Haarlem | `1A` | Visby |
| `07` | Harlingen | `1B` | Helsinki |
| `08` | Groningen | `1C` | Stettin |
| `09` | Cologne | `1D` | Ruegenwald |
| `0A` | Bremen | `1E` | Gdansk |
| `0B` | Ripen | `1F` | Torun |
| `0C` | Hamburg | `20` | Koenigsberg |
| `0D` | Flensburg | `21` | Memel |
| `0E` | Luebeck | `22` | Windau |
| `0F` | Rostock | `23` | Riga |
| `10` | Bergen | `24` | Pernau |
| `11` | Stavanger | `25` | Reval |
| `12` | Toensberg | `26` | Ladoga |
| `13` | Oslo | `27` | Novgorod |

The table provides 40 raw town identities. The maximum number simultaneously present in all game scenarios is a separate question and is not asserted here.

## Facility type IDs

| ID | Type |
|---:|---|
| `00` | Militia |
| `01` | Shipyard |
| `02` | Unknown |
| `03` | Weaponsmith |
| `04` | Hunting Lodge |
| `05` | Fisherman's Hut |
| `06` | Brewery |
| `07` | Workshop |
| `08` | Apiary |
| `09` | Farm Grain |
| `0A` | Farm Cattle |
| `0B` | Sawmill |
| `0C` | Weaving Mill |
| `0D` | Saltworks |
| `0E` | Iron Smelter |
| `0F` | Farm Sheep |
| `10` | Vineyard |
| `11` | Pottery |
| `12` | Brickworks |
| `13` | Pitchmaker |
| `14` | Farm Hemp |
