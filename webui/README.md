# Nebula POV-POI Web UI

A comprehensive React-based web interface for controlling and configuring the Nebula POI wireless persistence-of-vision system.

## Features

- **Dashboard**: Fleet control interface for managing multiple POV devices
- **Image Lab**: Upload images or generate procedural art, create sequences
- **Advanced Settings**: Configure hardware parameters (LED count, GPIO pins, refresh rates)
- **Code Viewers**: Browse ESP32 and Teensy Arduino sketch templates
- **Wiring Guide**: Visual reference for hardware connections
- **Firmware Manager**: OTA update interface (UI ready, backend pending)

## Prerequisites

- Node.js 18+ and npm
- POV-POI hardware connected to `POV-POI-WiFi` network (192.168.4.1)

## Installation

```bash
cd webui
npm install
```

## Development

Run the development server with API proxy to ESP32:

```bash
npm run dev
```

The dev server will start on `http://localhost:3000` and proxy API requests to `http://192.168.4.1`.

## Building for Production

Build the optimized production bundle:

```bash
npm run build
```

The output will be in the `dist/` directory, optimized for ESP32's SPIFFS/LittleFS filesystem with:
- Single-chunk bundle (no code splitting for embedded systems)
- Minified JavaScript and CSS
- Optimized assets

## Deployment to ESP32

### Option 1: Manual SPIFFS Upload (Recommended for Development)

1. Build the production bundle: `npm run build`
2. Install the ESP32 filesystem uploader plugin for Arduino IDE or PlatformIO
3. Copy contents of `dist/` to the ESP32's `data/` directory
4. Upload to SPIFFS/LittleFS using the uploader tool

**Arduino IDE:**
```bash
# Install ESP32 Sketch Data Upload plugin
# Tools > ESP32 Sketch Data Upload
```

**PlatformIO:**
```bash
cd ../esp32_firmware
pio run --target uploadfs
```

### Option 2: Automated Build Script (Coming Soon)

A deployment script will be added to automate the build and filesystem upload process.

## Configuration

### Tailwind CSS Setup

Currently using Tailwind CDN for development. For production deployment with custom builds:

1. Install Tailwind as a dev dependency:
```bash
npm install -D tailwindcss postcss autoprefixer
npx tailwindcss init -p
```

2. Configure `tailwind.config.js`:
```javascript
export default {
  content: [
    "./index.html",
    "./components/**/*.{js,ts,jsx,tsx}",
    "./App.tsx",
  ],
  theme: {
    extend: {},
  },
  plugins: [],
}
```

3. Create `src/styles.css`:
```css
@tailwind base;
@tailwind components;
@tailwind utilities;
```

4. Import in `index.html` instead of CDN link

## API Endpoints

The web UI communicates with the ESP32 firmware via REST API:

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/status` | GET | Get device status |
| `/api/mode` | POST | Set display mode (JSON: `{mode, index}`) |
| `/api/brightness` | POST | Set brightness (JSON: `{brightness}`) |
| `/api/framerate` | POST | Set frame rate (JSON: `{framerate}`) |
| `/api/image` | POST | Upload image (multipart) |
| `/api/sd/load` | POST | Load image from SD card |
| `/api/device/config` | GET/POST | Get/set hardware config |

See `docs/API.md` in the root directory for complete API documentation.

## Fleet Configuration

Default fleet IPs are defined in `components/Dashboard.tsx`:

```typescript
const FLEET_IPS = ['192.168.4.1', '192.168.4.2'];
```

Modify this array to match your multi-device setup.

## Project Structure

```
webui/
├── components/          # React components
│   ├── Dashboard.tsx    # Fleet control interface
│   ├── ImageLab.tsx     # Image processing and sequences
│   ├── AdvancedSettings.tsx  # Hardware configuration
│   ├── CodeViewer.tsx   # Arduino sketch display
│   ├── WiringGuide.tsx  # Hardware connection guide
│   └── FirmwareManager.tsx   # OTA updates (UI only)
├── constants.tsx        # Arduino sketch templates
├── types.ts            # TypeScript interfaces
├── App.tsx             # Main application
├── index.tsx           # Entry point
├── index.html          # HTML template
├── vite.config.ts      # Vite build configuration
└── package.json        # Dependencies

```

## Known Limitations

### OTA Firmware Updates
The Firmware Manager UI is complete, but the backend OTA endpoints (`/api/ota/esp32` and `/api/ota/teensy`) are not yet implemented in the ESP32 firmware. Use USB and Arduino IDE/PlatformIO for firmware updates until these endpoints are added.

### AI Assistant
The AI Assistant feature requires a Google Gemini API key. For security, the key should NOT be embedded in the client-side code. To enable this feature in production, implement a backend proxy endpoint on the ESP32 that forwards requests to the Gemini API.

### Tailwind CDN
Currently using Tailwind via CDN, which is not optimal for production (large bundle, no tree-shaking). Follow the Tailwind CSS Setup section above to switch to a proper build configuration.

## Development Tips

### Testing Without Hardware
You can test the UI without connected hardware by:
1. Running `npm run dev` (dev server includes API proxy)
2. API calls will fail gracefully with timeout errors
3. UI components will still render and function

### Adding New Components
1. Create component in `components/`
2. Import in `App.tsx`
3. Add to navigation menu
4. Update `ViewMode` enum in `types.ts`

### Debugging API Calls
Use browser DevTools Network tab to inspect API requests/responses. The development server proxy logs all API calls to the console.

## Troubleshooting

### "Cannot connect to device"
- Ensure you're connected to `POV-POI-WiFi` network
- Verify ESP32 is powered and running firmware
- Check that ESP32 IP is `192.168.4.1` (or update `DEVICE_IP` constants)

### "Module not found" errors
- Run `npm install` to ensure all dependencies are installed
- Clear node_modules and reinstall: `rm -rf node_modules package-lock.json && npm install`

### Build fails
- Check Node.js version: `node --version` (should be 18+)
- Clear Vite cache: `rm -rf node_modules/.vite`
- Try a clean build: `rm -rf dist && npm run build`

### Tailwind styles not working
- If using CDN, ensure the CDN link in `index.html` is not blocked
- For production, follow the Tailwind CSS Setup instructions above

## Contributing

When making changes to the web UI:
1. Test locally with `npm run dev`
2. Build and verify production bundle: `npm run build && npm run preview`
3. Ensure changes are compatible with ESP32's limited resources
4. Update this README if adding new features or configuration

## License

Same as parent project (see root LICENSE file).
