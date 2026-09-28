# Roadmap

## 0.1.x — stabilization

- Validate the current release candidate on multiple campaigns.
- Test more Patrician III language/executable variants.
- Improve graceful failure reporting when a hook signature does not match.
- Identify and credit the exact Cheat Engine table/source used during research.

## 0.2.0 — configuration

Planned `RealEvents.ini` support:

```ini
[Events]
MinActive=1
MaxActive=5
MinIntervalDays=30
MaxIntervalDays=90
MinDurationMonths=1
MaxDurationMonths=12

[Crisis]
Weight=60
MinProductionPercent=40
MaxProductionPercent=60

[Boom]
Weight=40
MinProductionPercent=140
MaxProductionPercent=160

[Informer]
Enabled=1
```

This file is intentionally **not active in v0.1.0**. The first public candidate keeps the already tested constants rather than adding untested runtime configuration at the same time as the release cleanup.

## Future event ideas

The event engine is intentionally broader than only crises and booms. Possible future categories include:

- shortages or surpluses affecting specific wares;
- temporary regional modifiers;
- weather-related production effects;
- port or trade disruptions;
- city-specific narrative events;
- event chains that create follow-up consequences.

Each new event type should be documented separately and should avoid replacing vanilla game information when it can be integrated naturally.
