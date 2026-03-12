# Web UI Development

Key knowledge for working on the React web interface at `esp32_firmware/webui/`.

## Stack

- React + TypeScript + Vite + Tailwind CSS (PostCSS, not CDN)
- Production build: single-chunk bundle (~325 KB) for ESP32 SPIFFS
- No ESLint — use `npx tsc --noEmit` for static analysis

## Project Layout

```
esp32_firmware/webui/
├── src/
│   ├── App.tsx           # Main app structure
│   ├── index.tsx         # Entry point (imports CSS here, not in HTML)
│   ├── components/       # UI components (PascalCase filenames)
│   ├── types.ts          # TypeScript interfaces
│   └── constants.tsx     # API URLs, configuration
├── vite.config.ts        # Dev proxy points to 10.100.9.230 (real device)
├── tailwind.config.js
└── postcss.config.js
```

## Conventions

- **Components**: PascalCase (`Dashboard.tsx`)
- **Functions**: camelCase (`handleBrightnessChange`)
- **Constants**: UPPER_CASE (`API_BASE_URL`)
- **Types**: PascalCase interfaces in `types.ts`

## Best Practices

- **Debounce** slider/input API calls (200-300 ms) to avoid flooding the ESP32
- **Use refs** for timer/interval effects to prevent unnecessary re-renders
- **Import CSS** in `index.tsx`, not in HTML
- Build produces a **single chunk** (no code splitting — embedded target)

## API Base URL

The ESP32 serves at `http://192.168.4.1`. During development the Vite proxy forwards `/api/*` to the real device IP. Without hardware, API calls fail but the UI is fully interactive.

## Key Endpoints

| Endpoint | Method | Body |
|----------|--------|------|
| `/api/status` | GET | — |
| `/api/brightness` | POST | `{"brightness": 0-255}` |
| `/api/framerate` | POST | `{"framerate": 10-120}` |
| `/api/mode` | POST | `{"mode": 0-4, "index": n}` |
| `/api/pattern` | POST | `{"type": 0-17, "color1": "#hex", "speed": 1-255}` |
| `/api/image` | POST | multipart file upload |
