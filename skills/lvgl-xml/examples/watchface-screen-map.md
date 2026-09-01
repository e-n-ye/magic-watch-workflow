# Watchface Screen Map

Approved concept: `build/design-evidence/watchface-v2-6x7/draft-01.png`
Target canvas: `240x280` portrait, with a `16px` horizontal safe inset.

The map keeps the visual hierarchy intentionally sparse for the F411 display:

| Region | Stable objects | Purpose |
| --- | --- | --- |
| Header | `page_brand`, `watchface_battery`, `watchface_status` | Product label, battery/degraded state, sensor readiness |
| Time | `page_title`, `watchface_time_rule` | Dominant RTC time and separator |
| Date | `watchface_weekday`, `watchface_date` | Weekday and month/day, side by side |
| Summary | `watchface_summary`, `watchface_steps`, `watchface_steps_label`, `watchface_summary_hint` | Single touch target for steps/status summary |
| Footer | `page_hint` | Encoder navigation hint |

All child coordinates are relative to their named parent. The summary card is
the only interactive region in this first concept and has a `208x58` hitbox.
The map uses the longest V1 weekday (`WEDNESDAY`) so the runtime replacement
cannot unexpectedly collide with the date label. Keep the map synchronized with
any user-approved Editor adjustment, then run `screen_map_validate.py` before
translating it into XML; a failed validation blocks generation.
