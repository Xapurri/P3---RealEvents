# Reverse-engineering notes

This document records the discoveries required by Real Events. It intentionally separates upstream knowledge from findings verified during development.

## Evidence labels

- **Upstream** — already documented by P3Modding or existing community research.
- **Confirmed by Real Events** — independently verified at runtime while developing this mod.
- **Observed** — reproducible, but the complete underlying meaning is not yet established.
- **Hypothesis** — not yet sufficiently verified.

## Game time

**Upstream + confirmed by Real Events**

`GameWorld + 0x14` contains the game timestamp.

P3Modding documents:

```text
TICKS_PER_DAY  = 256
TICKS_PER_YEAR = 93440
```

Real Events runtime testing observed consecutive calendar days differing by `0x100`, therefore:

```cpp
absoluteGameDay = gametime >> 8;
```

Real Events uses this absolute day count instead of polling only the day-of-month byte. This is important when the player uses fast-forward.

## Town production dispatcher

**Confirmed by Real Events**

Entry:

```text
VA  0x005101D0
RVA 0x001101D0
```

The facility entry being processed contains:

```text
+0x00 uint32_t efficiency
+0x04 uint16_t employees
+0x06 uint8_t  facilityType
+0x07 uint8_t  townIndex
```

Temporarily changing `+0x00` changes the amount produced by the current town facility.

The original value is restored at all three confirmed dispatcher exits:

```text
0x0051077F
0x005107B4
0x00510876
```

This lets Real Events affect production without permanently modifying the facility object.

### Confirmed shared facility routines

Observed jump-table destinations include:

```text
Type 0x05 Fisherman's Hut -> 0x0050E690
Type 0x07 Workshop        -> 0x0050FBC0
Type 0x12 Brickworks      -> 0x0050F2E0
Type 0x13 Pitchmaker      -> 0x0050F390
```

Examples of town commodity accumulators observed during tracing:

```text
Fish       -> Town + 0xCC
Iron goods -> Town + 0xF4
Pitch      -> Town + 0x100
Bricks     -> Town + 0x110
```

## Private / Trader production

**Confirmed by Real Events**

Dispatcher area:

```text
0x004D3782
```

Hook used by Real Events:

```text
VA  0x004D3798
RVA 0x000D3798
```

At the hook:

```asm
mov al,[esi+06]
cmp eax,14
```

`ESI` points to a MerchantFacility-compatible entry using the same leading layout:

```text
+0x00 efficiency
+0x04 employees
+0x06 facility type
+0x07 town index
```

Common exit used by Real Events:

```text
VA  0x004D3AE3
RVA 0x000D3AE3
```

Runtime tests confirmed that temporary efficiency scaling affects only the private facility currently being processed, including AI-owned facilities.

Examples verified during development:

- Player Workshop at `0x400 -> 0x200`: player Iron Goods production approximately halved.
- AI Workshop at `0x400 -> 0x200`: AI production approximately halved without changing the player's production.
- AI Pitchmaker behaved equivalently.
- Town and private hooks can operate together during the same city event.

## Tavern Informer text

**Confirmed by Real Events**

The Informer conversation renderer reaches:

```asm
0x005D138F  mov eax,[esi+0x216C]
0x005D1395  lea ecx,[esp+0x18]
0x005D1399  push eax
0x005D139A  push 0x006C66BC   ; "\f1_%s"
0x005D139F  push ecx
0x005D13A0  call 0x0064DF80
```

At `0x005D1399`, `EAX` was verified to point to the complete vanilla Informer text buffer, containing multiple normal rumours concatenated into one string.

A breakpoint at this point was tested across other game screens and only triggered when entering the Informer conversation.

Real Events therefore hooks the six bytes beginning at `0x005D1399`, builds a separate buffer containing:

```text
vanilla Informer text
+
one Real Events rumour
```

and then replays the original pushes before continuing at `0x005D139F`.

The vanilla buffer itself is never modified.

## Event semantics

**Real Events design, not a vanilla game structure**

Current event types:

```text
0 NONE
1 CRISIS
2 BOOM
```

Current defaults:

```text
Crisis probability weight: 60
Boom probability weight:   40

Crisis production: 40–60%
Boom production:  140–160%

Duration:          1–12 x 30 game days
Evaluation:        every 30–90 game days
Maximum event cities: 5
```

These are design parameters of the mod, not reverse-engineered vanilla mechanics.

## Still unresolved

- Exact semantics of every unknown field in the Facility/MerchantFacility entry.
- Full origin/construction pipeline for vanilla Informer rumours before the final text buffer.
- Complete compatibility matrix across game languages and executable revisions.
- Exact vanilla calendar month lengths as a replacement for the current 30-day event-duration approximation.
