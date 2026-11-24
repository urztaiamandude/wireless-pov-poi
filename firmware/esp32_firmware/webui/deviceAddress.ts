import { isNativePlatform, loadPreference, savePreference } from './nativeFeatures';

/** Access-point address of a poi that has not been given another host. */
export const DEFAULT_DEVICE_HOST = '192.168.4.1';

const PREF_KEY = 'deviceHost';
const STORAGE_KEY = 'nebula-device-host';

const HOST_RE = /^(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)(?:\.(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?))*$/;

/**
 * Accept an IPv4 address or hostname, including povpoi.local and an optional port.
 * A pasted http(s) URL is reduced to the host. The caller always uses http.
 */
export function normalizeDeviceHost(input: string): string | null {
  let value = input.trim();
  if (!value) return null;
  value = value.replace(/\/+$/, '');
  value = value.replace(/^https?:\/\//i, '');
  value = value.split('/')[0] ?? '';
  value = value.split('?')[0] ?? '';
  if (!value || value.includes('@') || /\s/.test(value)) return null;

  let host = value;
  let port = '';
  const colon = value.lastIndexOf(':');
  if (colon !== -1) {
    host = value.slice(0, colon);
    port = value.slice(colon + 1);
    if (!/^\d+$/.test(port)) return null;
    const portNum = Number(port);
    if (portNum < 1 || portNum > 65535) return null;
  }
  if (!HOST_RE.test(host)) return null;
  return port ? `${host}:${port}` : host;
}

function readLocalHost(): string | null {
  try {
    return normalizeDeviceHost(localStorage.getItem(STORAGE_KEY) ?? '');
  } catch {
    return null;
  }
}

function writeLocalHost(host: string): void {
  try {
    localStorage.setItem(STORAGE_KEY, host);
  } catch {
    // Private mode or a WebView that blocks storage. Preferences still holds the value.
  }
}

let cachedHost = readLocalHost() ?? DEFAULT_DEVICE_HOST;
const listeners = new Set<(host: string) => void>();

export function getDeviceHost(): string {
  return cachedHost;
}

export function subscribeDeviceHost(listener: (host: string) => void): () => void {
  listeners.add(listener);
  return () => {
    listeners.delete(listener);
  };
}

function publish(host: string): void {
  cachedHost = host;
  writeLocalHost(host);
  listeners.forEach((listener) => listener(host));
}

/** Load the saved host. Preferences wins; localStorage covers the next cold start. */
export async function loadDeviceHost(): Promise<string> {
  const fromPrefs = normalizeDeviceHost((await loadPreference(PREF_KEY)) ?? '');
  const host = fromPrefs ?? readLocalHost() ?? DEFAULT_DEVICE_HOST;
  publish(host);
  return host;
}

/** Persist a host. Returns null when the text is not an address or hostname. */
export async function saveDeviceHost(input: string): Promise<string | null> {
  const host = normalizeDeviceHost(input);
  if (!host) return null;
  await savePreference(PREF_KEY, host);
  publish(host);
  return host;
}

/**
 * Base URL for /api requests, with no trailing slash.
 * Native shell: http://<saved host>. The ESP32 has no TLS.
 * Poi page and Vite dev: '' so requests stay relative and the dev proxy still applies.
 */
export function getDeviceBase(): string {
  if (isNativePlatform()) {
    return `http://${cachedHost}`;
  }
  return '';
}

export async function testDeviceConnection(input?: string): Promise<{ ok: boolean; detail: string }> {
  const host = normalizeDeviceHost(input ?? cachedHost);
  if (!host) {
    return { ok: false, detail: 'Enter an IPv4 address or hostname, such as 192.168.4.1 or povpoi.local.' };
  }
  try {
    const res = await fetch(`http://${host}/api/status`, { signal: AbortSignal.timeout(4000) });
    if (!res.ok) return { ok: false, detail: `http://${host} answered HTTP ${res.status}.` };
    return { ok: true, detail: `Connected to http://${host}.` };
  } catch {
    return { ok: false, detail: `Could not reach http://${host}. Join POV-POI-WiFi and check the address.` };
  }
}
