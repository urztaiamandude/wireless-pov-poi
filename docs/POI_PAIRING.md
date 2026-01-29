# POV Poi Pairing and Synchronization Guide

## Overview

The Nebula POV Poi system supports **peer-to-peer pairing and synchronization** between multiple poi devices. This allows you to create perfectly synchronized light shows with matching patterns, images, and settings across a pair (or more) of poi devices.

## Key Features

### Peer-to-Peer Architecture
- ✅ **No Master/Slave** - All devices are equal peers
- ✅ **Independent Operation** - Each device works standalone
- ✅ **Automatic Discovery** - Devices find each other automatically
- ✅ **Bidirectional Sync** - Data flows both ways
- ✅ **Local Storage** - Each device maintains its own copies

### What Gets Synchronized
- **Images** - Custom uploaded POV images
- **Patterns** - Pattern configurations and settings
- **Settings** - Brightness, frame rate, current mode
- **Sequences** - Sequence configurations (if supported)

### What Doesn't Sync
- **Live Drawing** - Real-time mode is device-specific
- **WiFi Credentials** - Each device keeps its own AP settings
- **Device Identity** - Device IDs and names remain unique

## How It Works

### Device Identity

Each POV poi device has:
1. **Unique Device ID** - Automatically generated MAC-based ID
2. **Device Name** - User-configurable friendly name (e.g., "Left Poi", "Right Poi")
3. **Sync Group** - Optional group ID for multi-device setups

### Discovery Process

```
┌──────────────┐         ┌──────────────┐
│  POI Device 1│         │  POI Device 2│
│  (Left Poi)  │         │  (Right Poi) │
└──────┬───────┘         └──────┬───────┘
       │                        │
       │  1. Broadcast Beacon   │
       ├───────────────────────>│
       │                        │
       │  2. Response + Info    │
       │<───────────────────────┤
       │                        │
       │  3. Establish Connection
       │<──────────────────────>│
       │                        │
       │  4. Sync Data          │
       │<──────────────────────>│
       │                        │
```

### Sync Protocol

#### Sync Modes

**Manual Sync**
- Triggered by user via web interface
- One-time synchronization operation
- User controls what gets synced

**Auto-Sync** (Optional)
- Automatic sync when peer detected
- Configurable interval (e.g., every 30 seconds)
- Can be enabled/disabled per device

### Conflict Resolution

When both devices have different versions of the same content:

1. **Timestamp-Based** - Newest version wins
2. **User Override** - Manual sync allows choosing direction
3. **Merge Strategy** - Combines unique content from both devices

## Setup Guide

### Step 1: Name Your Devices

Each device should have a unique, identifiable name:

**Device 1 (Left Poi):**
1. Connect to `POV-POI-WiFi` (or custom SSID)
2. Open `http://192.168.4.1`
3. Go to Settings → Device Info
4. Set name to "Left Poi"
5. Save settings

**Device 2 (Right Poi):**
1. Connect to its WiFi AP (will have different SSID suffix)
2. Open `http://192.168.4.1`
3. Go to Settings → Device Info
4. Set name to "Right Poi"
5. Save settings

### Step 2: Configure Sync Group (Optional)

If you want automatic pairing:
1. Set same sync group ID on both devices
2. Example: "MySyncGroup" or "PoiPair01"
3. Devices with matching group IDs will auto-pair

### Step 3: Discover Peers

**Method A: Automatic Discovery**
1. Both devices must be powered on
2. Wait 10-30 seconds for discovery
3. Check "Sync Status" on web interface
4. Peer devices will appear in list

**Method B: Manual Addition**
1. Go to Sync → Add Peer
2. Enter peer device IP address
3. Device will be added to peer list

### Step 4: Perform Initial Sync

1. Click "Sync Now" on either device
2. Choose sync direction:
   - **Bidirectional** (default) - Merge both devices
   - **From This Device** - Push to peer
   - **To This Device** - Pull from peer
3. Wait for sync to complete
4. Verify both devices have same content

## Using Paired Poi

### Synchronized Operation

Once paired and synced:

1. **Change Pattern on Either Device**
   - Select pattern on Device 1
   - Device 2 will see the change (if auto-sync enabled)
   - Or manually sync to propagate

2. **Upload Image to One Device**
   - Upload image to Device 1
   - Image automatically available on Device 2 after sync
   - Both devices can display the same image

3. **Adjust Settings**
   - Change brightness on Device 1
   - Sync to match Device 2's brightness
   - Perfect synchronized appearance

### Independent Operation

Devices remain fully independent:

1. **No Connection Required**
   - Each device works without peer
   - All content stored locally
   - Sync when convenient

2. **Different Content**
   - Devices can show different patterns
   - Useful for contrasting effects
   - Sync is optional, not required

## Sync API Reference

### REST Endpoints

#### Get Sync Status
```
GET /api/sync/status
```

**Response:**
```json
{
  "deviceId": "A1:B2:C3:D4:E5:F6",
  "deviceName": "Left Poi",
  "syncGroup": "PoiPair01",
  "peers": [
    {
      "deviceId": "F6:E5:D4:C3:B2:A1",
      "deviceName": "Right Poi",
      "ipAddress": "192.168.4.2",
      "lastSeen": 1706518800,
      "online": true
    }
  ],
  "autoSync": true,
  "syncInterval": 30
}
```

#### Discover Peers
```
POST /api/sync/discover
```

**Response:**
```json
{
  "status": "ok",
  "peersFound": 1,
  "peers": [
    {
      "deviceId": "F6:E5:D4:C3:B2:A1",
      "deviceName": "Right Poi",
      "ipAddress": "192.168.4.2"
    }
  ]
}
```

#### Trigger Sync
```
POST /api/sync/execute
```

**Request Body:**
```json
{
  "peerId": "F6:E5:D4:C3:B2:A1",
  "direction": "bidirectional",
  "syncImages": true,
  "syncPatterns": true,
  "syncSettings": true
}
```

**Response:**
```json
{
  "status": "ok",
  "itemsSynced": 5,
  "imagesAdded": 2,
  "patternsAdded": 1,
  "settingsUpdated": true
}
```

#### Get Sync Data
```
GET /api/sync/data
```

**Response:**
```json
{
  "images": [
    {
      "id": "img_001",
      "name": "spiral.png",
      "width": 31,
      "height": 64,
      "timestamp": 1706518800,
      "size": 5952
    }
  ],
  "patterns": [
    {
      "id": "pat_001",
      "type": 0,
      "name": "Rainbow Fast",
      "timestamp": 1706518700
    }
  ],
  "settings": {
    "brightness": 128,
    "framerate": 60,
    "mode": 2,
    "index": 0,
    "timestamp": 1706518900
  }
}
```

#### Push Sync Data
```
POST /api/sync/push
```

**Request Body:**
```json
{
  "images": [...],
  "patterns": [...],
  "settings": {...}
}
```

## Troubleshooting

### Devices Not Discovering Each Other

**Problem:** Peer devices not appearing in sync list

**Solutions:**
1. Ensure both devices are powered on
2. Check WiFi signal strength
3. Verify devices are on same subnet
4. Try manual IP address entry
5. Restart both devices and wait 30 seconds

### Sync Fails or Times Out

**Problem:** Sync operation fails or times out

**Solutions:**
1. Check network connectivity
2. Ensure peer device is responding (check /api/status)
3. Reduce sync data size (fewer images)
4. Try syncing in smaller batches
5. Check available storage space on both devices

### Content Not Matching After Sync

**Problem:** Devices show different content despite sync

**Solutions:**
1. Check sync logs for errors
2. Verify timestamps on both devices
3. Manually trigger bidirectional sync
4. Clear cache and re-sync
5. Check for storage errors on either device

### Auto-Sync Not Working

**Problem:** Automatic sync not triggering

**Solutions:**
1. Verify auto-sync is enabled on both devices
2. Check sync interval setting
3. Ensure peer device is online
4. Review sync logs for errors
5. Manually sync to test connectivity

## Advanced Configuration

### Custom Sync Intervals

Edit ESP32 firmware to change sync timing:

```cpp
#define AUTO_SYNC_INTERVAL 30000  // 30 seconds (in milliseconds)
#define PEER_DISCOVERY_INTERVAL 60000  // 60 seconds
#define PEER_TIMEOUT 120000  // 2 minutes
```

### Network Configuration for Multiple Devices

For more than 2 devices:

1. Each device needs unique WiFi SSID suffix
2. All devices should be on same subnet
3. Use sync group to organize devices
4. Consider using external WiFi router as bridge

### Storage Management

Each device stores sync data in SPIFFS:

- Images: `/sync/images/`
- Patterns: `/sync/patterns/`
- Metadata: `/sync/metadata.json`

Monitor storage usage:
```
GET /api/storage/info
```

## Security Considerations

### Network Security

- Each device has its own WiFi AP
- Password protected (default: `povpoi123`)
- Local network only (no internet)
- Change default password for security

### Sync Security

- Sync only works on local network
- No external sync servers
- Data never leaves local network
- Consider VPN for remote sync (advanced)

## Performance Tips

### Optimize Sync Speed

1. **Reduce Image Sizes** - Smaller images sync faster
2. **Selective Sync** - Only sync what you need
3. **Schedule Syncs** - Sync during idle time
4. **Batch Operations** - Sync multiple items at once

### Battery Life Considerations

- Auto-sync uses more power
- Disable auto-sync for longer battery life
- Manual sync only when needed
- Adjust sync interval to balance convenience and battery

## Examples

### Example 1: Basic Pair Setup

```bash
# Device 1
curl -X POST http://192.168.4.1/api/sync/discover

# Device 2
curl -X POST http://192.168.4.2/api/sync/discover

# Sync from Device 1 to Device 2
curl -X POST http://192.168.4.1/api/sync/execute \
  -H "Content-Type: application/json" \
  -d '{"peerId":"F6:E5:D4:C3:B2:A1","direction":"bidirectional"}'
```

### Example 2: Automated Sync Script

```bash
#!/bin/bash
# sync_poi_pair.sh

# Discover peers
curl -s http://192.168.4.1/api/sync/discover

# Wait for discovery
sleep 2

# Execute sync
curl -X POST http://192.168.4.1/api/sync/execute \
  -H "Content-Type: application/json" \
  -d '{
    "peerId": "auto",
    "direction": "bidirectional",
    "syncImages": true,
    "syncPatterns": true,
    "syncSettings": false
  }'

echo "Sync complete!"
```

## FAQ

**Q: Can I sync more than 2 poi devices?**  
A: Yes! The system supports multiple devices. Use sync groups to organize them.

**Q: Does sync work without WiFi?**  
A: No, devices need WiFi network connectivity to sync. But each device works independently without sync.

**Q: Can I sync with poi on different WiFi networks?**  
A: Not directly. Devices must be on same local network. Use WiFi router as bridge for multiple APs.

**Q: How much storage is needed for sync?**  
A: Depends on content. Each image ~6KB, patterns minimal. ESP32-S3 with 16MB has plenty of space.

**Q: Can I undo a sync?**  
A: Sync doesn't have undo, but you can sync back from backup device or re-upload original content.

**Q: Does sync interfere with performance?**  
A: No. Sync happens in background. Display performance is not affected during sync.

**Q: Can I sync while performing?**  
A: Yes, but manual sync during performance is not recommended. Use pre-performance sync or auto-sync during idle times.

## Related Documentation

- [API Documentation](API.md) - Complete API reference
- [README](../README.md) - Project overview
- [ARCHITECTURE](../ARCHITECTURE.md) - System architecture
- [TROUBLESHOOTING](../TROUBLESHOOTING.md) - General troubleshooting

---

**Last Updated:** 2026-01-29  
**Version:** 1.0  
**Feature Status:** In Development
