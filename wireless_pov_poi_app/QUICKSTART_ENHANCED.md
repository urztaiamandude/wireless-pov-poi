# Quick Start Guide - Enhanced Flutter App v2.0

Get started with the enhanced Wireless POV Poi app features in 5 minutes!

## New Features at a Glance

### 🎨 Procedural Art Generator
Create organic flowing patterns or geometric designs with adjustable complexity and color palettes. One-tap save to library.

### 📚 Enhanced Pattern Library
Search, filter, and manage patterns with grid/list views. Batch operations for efficient organization.

### 🎬 Sequence Editor  
Build complex sequences with drag-to-reorder segments. Preview locally before uploading.

### 🚀 Fleet Management
Control multiple POV Poi devices simultaneously with synchronized or individual modes.

### ⚙️ Advanced Settings
Quick-access presets for brightness (6 levels) and speed (6 levels). Auto-connect and display options.

---

## First Time with Enhanced Features

### 1. Generate Your First Procedural Pattern

**From Home Page:**
```
Tap "Generate Art" (purple button)
  ↓
Select pattern type: Organic or Geometric
  ↓
Adjust complexity slider (try 10)
  ↓
Tap "Regenerate" for variations
  ↓
Tap 💾 Save when satisfied
```

**Result**: Pattern appears in Pattern Library

### 2. Explore Pattern Library

**From Home Page:**
```
Tap "Pattern Library" (blue button)
  ↓
See all saved patterns in grid view
  ↓
Tap 📋→📑 to switch to list view
  ↓
Use search bar to find patterns by name
  ↓
Tap ⋮ to sort by name/date/size
```

**Batch Operations:**
```
Long-press any pattern → Selection mode
  ↓
Tap multiple patterns to select
  ↓
Tap 🗑️ to delete all selected
```

### 3. Build a Simple Sequence

**From Home Page:**
```
Tap "Sequences" (deep purple button)
  ↓
Tap ➕ Add Segment (5 times)
  ↓
Configure each segment:
  - Pattern: Select bank (A/B/C) and slot (1-5)
  - Brightness: 15-25 recommended
  - Speed: 300-800ms for smooth animation
  - Duration: 3-5s per segment
  ↓
Drag segments using [☰] to reorder
  ↓
Tap ▶️ Play to preview
  ↓
Tap ⬆️ Upload to send to Poi
```

**Poi Behavior**: Plays sequence offline, loops automatically

### 4. Manage Multiple Devices

**Prerequisites**: Connect 2+ Poi devices via BLE

**From App Bar:**
```
Tap 📱 Devices icon (top-right)
  ↓
Enable "Synchronized Control" toggle
  ↓
Adjust Global Brightness slider
  ↓
Tap "Play" → All devices start together
```

**Individual Control:**
```
Disable sync mode
  ↓
Long-press a device card
  ↓
Select specific devices
  ↓
Adjust settings → Only selected devices affected
```

### 5. Use Brightness/Speed Presets

**From Home Page:**
```
Long-press "Wireless POV Poi" title
  ↓
Opens Advanced Settings
  ↓
Brightness Presets: Tap [5] [10] [15] [20] [25] [31]
  ↓
Speed Presets: Tap [100ms] [250ms] [500ms] etc.
```

**Effect**: Selected preset applied immediately to all controls

---

## Quick Reference

### Procedural Art Tips

| Pattern Type | Complexity | Effect |
|--------------|------------|--------|
| Organic | 4-8 | Flowing, smooth waves |
| Organic | 15-20 | Dense, complex curves |
| Geometric | 4-6 | Minimal, clean shapes |
| Geometric | 15-20 | Busy, intricate designs |

### Sequence Building Best Practices

✅ **DO**:
- Start with 3-5 segments (easier to manage)
- Vary brightness for visual interest
- Use 3-7s duration per segment
- Preview before uploading

❌ **DON'T**:
- Use all 70 segments at once (hard to manage)
- Max out brightness (battery drain)
- Use same pattern repeatedly
- Skip preview step

### Fleet Management Guidelines

**Sync Mode**: Best for performances where all Poi do the same thing
**Individual Mode**: Best for testing or creating layered effects

**Range**: Keep devices within 10m for reliable BLE connection

---

## Feature Comparison with WebUI

| Feature | WebUI | Enhanced App |
|---------|-------|--------------|
| Procedural Art | ✅ | ✅ (same) |
| Pattern Library | Basic | ✅ Enhanced (search, filter, batch) |
| Sequence Editor | Basic | ✅ Drag-to-reorder |
| Fleet Control | Basic | ✅ Sync + Individual modes |
| Offline Storage | ❌ | ✅ SQLite database |
| Presets | ❌ | ✅ 6 brightness + 6 speed |

---

## Troubleshooting Enhanced Features

### Procedural Art Won't Generate
- **Check**: Storage permissions granted
- **Fix**: Settings → Permissions → Enable Storage

### Pattern Search Not Working
- **Check**: Typing in search bar
- **Fix**: Tap search bar first, then type

### Sequence Preview Freezes
- **Check**: Too many segments (>50)
- **Fix**: Reduce to 20-30 segments for smooth preview

### Fleet Sync Delayed
- **Check**: Devices within range
- **Normal**: Slight delay (100-200ms) due to BLE

---

## Next Steps

1. **Master Procedural Art**: Try all complexity levels and color combinations
2. **Build a Complex Sequence**: Use 10+ segments with varying patterns
3. **Organize Library**: Search, filter, batch delete unused patterns
4. **Control Multiple Devices**: Practice sync modes for shows
5. **Customize Settings**: Set your favorite presets

---

## Documentation Quick Links

- **Full Features**: [ENHANCED_FEATURES.md](ENHANCED_FEATURES.md)
- **Architecture**: [ARCHITECTURE.md](ARCHITECTURE.md)
- **UI Mockups**: [UI_GUIDE.md](UI_GUIDE.md)
- **WebUI Comparison**: [COMPARISON.md](COMPARISON.md)
- **Original Quickstart**: [QUICKSTART_OLD.md](QUICKSTART_OLD.md)

---

**App Version**: v2.0 Enhanced
**Last Updated**: 2024-02-18

🎉 **Enjoy the enhanced features!** 🎉
