# App Navigation Flow & Feature Integration

## Navigation Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│                         Home Page                                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ App Bar                                                   │   │
│  │  - Title (long-press → Advanced Settings)                │   │
│  │  - Devices Icon → Fleet Management                       │   │
│  │  - Connection Indicators                                 │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Device Info Card                                          │   │
│  │  - LED Type, LED Count                                   │   │
│  │  - Max Pattern Size                                       │   │
│  │  - Connected Devices Count                               │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Quick Actions Grid (2x2)                                  │   │
│  │  ┌──────────────┐  ┌──────────────┐                      │   │
│  │  │ Generate Art │  │   Pattern    │                      │   │
│  │  │  (Purple)    │  │   Library    │                      │   │
│  │  │              │  │   (Blue)     │                      │   │
│  │  └──────────────┘  └──────────────┘                      │   │
│  │  ┌──────────────┐  ┌──────────────┐                      │   │
│  │  │  Sequences   │  │   Settings   │                      │   │
│  │  │(Deep Purple) │  │  (Orange)    │                      │   │
│  │  │              │  │              │                      │   │
│  │  └──────────────┘  └──────────────┘                      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Recent Activity                                           │   │
│  │  - View Library / Create New buttons                     │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
              │           │            │           │
      ┌───────┘           │            │           └───────┐
      │                   │            │                   │
      ▼                   ▼            ▼                   ▼
┌─────────────┐  ┌─────────────┐  ┌─────────┐  ┌─────────────────┐
│ Procedural  │  │  Pattern    │  │Sequence │  │    Advanced     │
│ Art         │  │  Library    │  │ Editor  │  │    Settings     │
│ Generator   │  │             │  │         │  │                 │
└─────────────┘  └─────────────┘  └─────────┘  └─────────────────┘
      │                   │            │                   │
      └───────┬───────────┴────────────┴───────────────────┘
              │
              ▼
      ┌─────────────────┐
      │  Pattern DB     │
      │  (SQLite)       │
      └─────────────────┘
```

## Feature Data Flow

### Pattern Creation & Management

```
User Input
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│ Pattern Sources                                              │
│                                                               │
│  1. Procedural Art Generator                                 │
│     - User adjusts complexity, color seed                    │
│     - Canvas renders in real-time                            │
│     - Save → Uint8List (PNG format)                          │
│                                                               │
│  2. Image Import (existing feature)                          │
│     - User selects image file                                │
│     - Converts to POV format                                 │
│     - Validates dimensions                                   │
│                                                               │
│  3. Text/Color/Stacked Creators (existing)                   │
│     - User inputs parameters                                 │
│     - Generates pattern programmatically                     │
│                                                               │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
                ┌───────────────┐
                │   DBImage     │
                │   Object      │
                │  - name       │
                │  - width      │
                │  - height     │
                │  - data       │
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │  PatternDB    │
                │  .insert()    │
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────────┐
                │  SQLite Database  │
                │  patterns table   │
                └───────┬───────────┘
                        │
                        ▼
        ┌───────────────────────────────┐
        │    Pattern Library Page       │
        │  - Search/Filter UI           │
        │  - Grid/List Display          │
        │  - Batch Operations           │
        │                               │
        │    User Actions:              │
        │  - View details               │
        │  - Select multiple            │
        │  - Delete                     │
        │  - Upload to Poi ────────────┼────┐
        └───────────────────────────────┘    │
                                              │
                                              ▼
                                    ┌─────────────────┐
                                    │  BLE Transfer   │
                                    │  via            │
                                    │  PoiHardware    │
                                    └─────────┬───────┘
                                              │
                                              ▼
                                    ┌─────────────────┐
                                    │  Wireless POV   │
                                    │  Poi Device     │
                                    └─────────────────┘
```

### Fleet Management Control Flow

```
┌──────────────────────────────────────────────────────────────┐
│                    Fleet Management Page                      │
│                                                                │
│  User Interface:                                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Sync Mode Toggle                                         │ │
│  │  [ON]  Synchronized  →  All devices                      │ │
│  │  [OFF] Individual    →  Selected devices only            │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Global Controls                                          │ │
│  │  Brightness: [===============o====] 20/31               │ │
│  │  Speed:      [========o============] 800ms              │ │
│  │                                                           │ │
│  │  [Play] [Pause] [Stop]                                   │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Device List                                              │ │
│  │  ┌─┬─────────────────┬──────────────┬──────────┐        │ │
│  │  │1│ POV_LEADER      │ Connected ● │  [Menu]  │        │ │
│  │  └─┴─────────────────┴──────────────┴──────────┘        │ │
│  │  ┌─┬─────────────────┬──────────────┬──────────┐        │ │
│  │  │2│ POV_FOLLOWER    │ Connected ● │  [Menu]  │        │ │
│  │  └─┴─────────────────┴──────────────┴──────────┘        │ │
│  └─────────────────────────────────────────────────────────┘ │
└───────────────────────┬──────────────────────────────────────┘
                        │
                        ▼
            ┌───────────────────────┐
            │  Command Processing   │
            │                       │
            │  if (syncMode):       │
            │    targets = all      │
            │  else:                │
            │    targets = selected │
            └───────┬───────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
        ▼                       ▼
┌───────────────┐      ┌───────────────┐
│ Device 1      │      │ Device 2      │
│ PoiHardware   │      │ PoiHardware   │
│               │      │               │
│ BLE Commands: │      │ BLE Commands: │
│ - Brightness  │      │ - Brightness  │
│ - Speed       │      │ - Speed       │
│ - Play/Pause  │      │ - Play/Pause  │
└───────┬───────┘      └───────┬───────┘
        │                      │
        ▼                      ▼
   [POV Poi 1]            [POV Poi 2]
```

### Sequence Editor Workflow

```
┌──────────────────────────────────────────────────────────────┐
│                    Sequence Editor Page                       │
│                                                                │
│  Summary:                                                     │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Segments: 5  │  Total Time: 15.0s  │  Max: 70           │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  Segment List (Reorderable):                                  │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ [1] Segment 1                                    [⋮]    │ │
│  │     Pattern: Bank A, Slot 1                             │ │
│  │     Brightness: [========o=======] 15/31                │ │
│  │     Speed:      [=====o===========] 500ms               │ │
│  │     Duration:   [======o=========] 3.0s                 │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ [2] Segment 2                            [PLAYING ▶]    │ │
│  │     Pattern: Bank B, Slot 3                             │ │
│  │     Brightness: [=============o==] 25/31                │ │
│  │     Speed:      [========o========] 800ms               │ │
│  │     Duration:   [=====o===========] 2.5s                │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  Actions:                                                     │
│  App Bar: [Play Preview] [Upload to Poi]                     │
│  FAB:     [+ Add Segment]                                     │
└───────────────────────┬──────────────────────────────────────┘
                        │
        ┌───────────────┴───────────────┐
        │                               │
        ▼                               ▼
┌───────────────┐              ┌────────────────┐
│ Preview Mode  │              │ Upload Mode    │
│               │              │                │
│ 1. Iterate    │              │ 1. Encode:     │
│    segments   │              │    - Segments  │
│ 2. Show       │              │    - Settings  │
│    active     │              │                │
│ 3. Delay by   │              │ 2. Send via    │
│    duration   │              │    BLE UART    │
│ 4. Loop       │              │                │
│               │              │ 3. Store in    │
│               │              │    device      │
│               │              │    EEPROM      │
└───────────────┘              └────────┬───────┘
                                        │
                                        ▼
                                ┌───────────────┐
                                │  POV Device   │
                                │  Runs offline │
                                │  sequence     │
                                └───────────────┘
```

### Advanced Settings Integration

```
┌──────────────────────────────────────────────────────────────┐
│                  Advanced Settings Page                       │
│                                                                │
│  Device Configuration:                                        │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Device Name:  [Wireless POV Poi      ] [Edit]          │ │
│  │ LED Type:      APA102                                   │ │
│  │ LED Count:     32                                       │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  Brightness Presets:                                          │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  [5] [10] [15*] [20] [25] [31]                          │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  Speed Presets:                                               │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  [100ms] [250ms] [500ms*] [1000ms] [1500ms] [2000ms]   │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  Display Settings:                                            │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ [✓] Auto-connect to devices                             │ │
│  │ [✓] Keep screen on                                      │ │
│  │ [ ] Show debug info                                     │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                │
│  [Reset to Defaults]                                          │
└───────────────────────┬──────────────────────────────────────┘
                        │
            ┌───────────┴───────────┐
            │                       │
            ▼                       ▼
    ┌───────────────┐      ┌────────────────┐
    │ SharedPrefs   │      │  App State     │
    │ (Persist)     │      │  (Runtime)     │
    │               │      │                │
    │ - presets     │      │ - applies to   │
    │ - toggles     │      │   all pages    │
    │ - device cfg  │      │ - immediate    │
    └───────────────┘      │   effect       │
                           └────────────────┘
```

## State Management Flow

```
┌──────────────────────────────────────────────────────────────┐
│                      Provider Pattern                         │
│                                                                │
│  ┌────────────┐                                               │
│  │   Model    │  ← ChangeNotifierProvider                    │
│  │            │                                               │
│  │ - connectedPoi: List<PoiHardware>                         │
│  │                                                             │
│  │ notifyListeners() when:                                    │
│  │  - Device connects/disconnects                            │
│  │  - Status changes                                          │
│  └─────┬──────┘                                               │
│        │                                                       │
│        │ Provider.of<Model>(context)                          │
│        │                                                       │
│  ┌─────▼──────────────────────────────────────────────────┐  │
│  │             All Pages Listen                            │  │
│  │                                                          │  │
│  │  HomePage               PatternLibraryPage               │  │
│  │  FleetManagementPage    SequenceEditorPage              │  │
│  │  ProceduralArtPage      AdvancedSettingsPage            │  │
│  │                                                          │  │
│  │  When model changes → UI rebuilds automatically         │  │
│  └──────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│                    Database Access Pattern                    │
│                                                                │
│  Any Page                                                     │
│      │                                                         │
│      │ creates instance                                       │
│      ▼                                                         │
│  ┌────────────┐                                               │
│  │ PatternDB  │                                               │
│  │            │                                               │
│  │ - getAllPatterns()                                         │
│  │ - insertPattern(DBImage)                                   │
│  │ - deletePattern(id)                                        │
│  │ - updatePattern(DBImage)                                   │
│  └─────┬──────┘                                               │
│        │                                                       │
│        ▼                                                       │
│  ┌─────────────┐                                              │
│  │   SQLite    │                                              │
│  │  Database   │                                              │
│  │             │                                              │
│  │  patterns   │                                              │
│  │  table      │                                              │
│  └─────────────┘                                              │
└──────────────────────────────────────────────────────────────┘
```

## User Journey Examples

### Journey 1: Create & Upload Procedural Art

```
1. Home Page
   └─> Tap "Generate Art"
       
2. Procedural Art Page
   ├─> Select "Organic" pattern type
   ├─> Adjust complexity slider (8 → 12)
   ├─> Adjust color seed for purple hues
   ├─> Tap "Regenerate" (preview updates)
   └─> Tap Save icon
       
3. Pattern Library (auto-navigates)
   ├─> New pattern appears at top
   ├─> Search "Procedural_organic"
   ├─> Tap pattern card
   └─> Tap "Upload to Poi"
       
4. BLE Transfer
   ├─> Progress indicator
   └─> Success notification
       
5. POV Device
   └─> Pattern stored in memory slot
```

### Journey 2: Manage Multiple Devices

```
1. Home Page
   └─> Tap Devices icon (app bar)
       
2. Fleet Management
   ├─> Enable "Synchronized Control"
   ├─> Adjust global brightness (15 → 25)
   ├─> Adjust global speed (500ms → 800ms)
   ├─> Tap "Play" button
   └─> Both devices start simultaneously
       
3. Individual Control
   ├─> Disable "Synchronized Control"
   ├─> Long-press Device 1 card
   ├─> Tap "Test Pattern"
   └─> Only Device 1 shows test pattern
```

### Journey 3: Build & Run Sequence

```
1. Home Page
   └─> Tap "Sequences"
       
2. Sequence Editor
   ├─> Tap "+" 5 times (add 5 segments)
   │
   ├─> Configure Segment 1:
   │   ├─ Pattern: Bank A, Slot 1
   │   ├─ Brightness: 20
   │   ├─ Speed: 300ms
   │   └─ Duration: 3s
   │
   ├─> Configure remaining segments...
   │
   ├─> Drag Segment 3 to position 2 (reorder)
   ├─> Tap Play icon (preview locally)
   └─> Tap Upload icon
       
3. BLE Transfer
   └─> Sequence uploaded to device
       
4. POV Device
   └─> Runs sequence autonomously
```

## Integration Points Summary

| Feature | Integrates With | Data Flow |
|---------|----------------|-----------|
| Procedural Art | PatternDB | Canvas → Uint8List → SQLite |
| Pattern Library | PatternDB, PoiHardware | SQLite ↔ UI, Upload via BLE |
| Fleet Management | Model, PoiHardware | Provider → BLE commands |
| Sequence Editor | PoiHardware | UI config → BLE encoded data |
| Advanced Settings | SharedPreferences, Model | Persist settings → Runtime state |
| Home Page | All Features | Navigation hub + status display |

## Performance Considerations

1. **Lazy Loading**: Pattern Library uses ListView.builder
2. **Debouncing**: Sliders delay API calls until user stops adjusting
3. **Provider Updates**: Only notify listeners when necessary
4. **Image Caching**: Patterns cached in memory for grid display
5. **Database Indexing**: Search queries use indexed columns
6. **Async Operations**: All BLE transfers are asynchronous
7. **Memory Management**: Dispose controllers and listeners properly

---

**Last Updated**: 2024-02-18
**Version**: v2.0
