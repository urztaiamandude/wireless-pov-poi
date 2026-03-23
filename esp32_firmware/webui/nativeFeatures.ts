/**
 * nativeFeatures.ts — Unified API for Capacitor native capabilities.
 *
 * Every helper tests for Capacitor availability at runtime so the same
 * React code works in a plain browser *and* inside the Android / iOS app.
 */

import { Capacitor } from '@capacitor/core';
import { Camera, CameraResultType, CameraSource } from '@capacitor/camera';
import { Haptics, ImpactStyle, NotificationType } from '@capacitor/haptics';
import { Network } from '@capacitor/network';
import { Preferences } from '@capacitor/preferences';
import { Share } from '@capacitor/share';
import { StatusBar, Style as StatusBarStyle } from '@capacitor/status-bar';
import { Toast } from '@capacitor/toast';
import { ScreenOrientation } from '@capacitor/screen-orientation';

/* ------------------------------------------------------------------ */
/*  Platform helpers                                                   */
/* ------------------------------------------------------------------ */

/** True when running inside a native Capacitor shell (Android / iOS). */
export const isNativePlatform = (): boolean => Capacitor.isNativePlatform();

/** Returns 'android', 'ios', or 'web'. */
export const getPlatform = (): string => Capacitor.getPlatform();

/* ------------------------------------------------------------------ */
/*  Camera                                                             */
/* ------------------------------------------------------------------ */

export interface CapturedPhoto {
  dataUrl: string;   // base64 data-URL usable with <img src>
  format: string;    // e.g. 'jpeg'
}

/**
 * Take a photo with the device camera and return it as a data-URL.
 * Falls back to `null` when camera is unavailable or the user cancels.
 */
export async function takePhoto(): Promise<CapturedPhoto | null> {
  try {
    const image = await Camera.getPhoto({
      quality: 90,
      allowEditing: false,
      resultType: CameraResultType.DataUrl,
      source: CameraSource.Camera,
      width: 800,
      correctOrientation: true,
    });
    if (image.dataUrl) {
      return { dataUrl: image.dataUrl, format: image.format ?? 'jpeg' };
    }
  } catch {
    // user cancelled or camera unavailable
  }
  return null;
}

/**
 * Pick a photo from the device gallery and return it as a data-URL.
 */
export async function pickPhoto(): Promise<CapturedPhoto | null> {
  try {
    const image = await Camera.getPhoto({
      quality: 90,
      allowEditing: false,
      resultType: CameraResultType.DataUrl,
      source: CameraSource.Photos,
      width: 800,
      correctOrientation: true,
    });
    if (image.dataUrl) {
      return { dataUrl: image.dataUrl, format: image.format ?? 'jpeg' };
    }
  } catch {
    // user cancelled
  }
  return null;
}

/* ------------------------------------------------------------------ */
/*  Haptics                                                            */
/* ------------------------------------------------------------------ */

export async function hapticImpact(style: 'light' | 'medium' | 'heavy' = 'medium'): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    const map = { light: ImpactStyle.Light, medium: ImpactStyle.Medium, heavy: ImpactStyle.Heavy };
    await Haptics.impact({ style: map[style] });
  } catch { /* unavailable */ }
}

export async function hapticNotification(type: 'success' | 'warning' | 'error' = 'success'): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    const map = { success: NotificationType.Success, warning: NotificationType.Warning, error: NotificationType.Error };
    await Haptics.notification({ type: map[type] });
  } catch { /* unavailable */ }
}

export async function hapticVibrate(): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    await Haptics.vibrate({ duration: 50 });
  } catch { /* unavailable */ }
}

/* ------------------------------------------------------------------ */
/*  Network                                                            */
/* ------------------------------------------------------------------ */

export interface NetworkInfo {
  connected: boolean;
  connectionType: string; // 'wifi' | 'cellular' | 'none' | 'unknown'
}

export async function getNetworkStatus(): Promise<NetworkInfo> {
  try {
    const status = await Network.getStatus();
    return { connected: status.connected, connectionType: status.connectionType };
  } catch {
    return { connected: false, connectionType: 'unknown' };
  }
}

/**
 * Register a listener for network status changes. Returns an unsubscribe function.
 */
export function onNetworkChange(callback: (info: NetworkInfo) => void): () => void {
  const handle = Network.addListener('networkStatusChange', (status) => {
    callback({ connected: status.connected, connectionType: status.connectionType });
  });
  return () => {
    handle
      .then((h) => h.remove())
      .catch(() => { /* listener registration or removal failed; ignore */ });
  };
}

/* ------------------------------------------------------------------ */
/*  Preferences (persistent key-value storage)                         */
/* ------------------------------------------------------------------ */

export async function savePreference(key: string, value: string): Promise<void> {
  try {
    await Preferences.set({ key, value });
  } catch { /* unavailable */ }
}

export async function loadPreference(key: string): Promise<string | null> {
  try {
    const { value } = await Preferences.get({ key });
    return value;
  } catch {
    return null;
  }
}

export async function removePreference(key: string): Promise<void> {
  try {
    await Preferences.remove({ key });
  } catch { /* unavailable */ }
}

/* ------------------------------------------------------------------ */
/*  Share                                                               */
/* ------------------------------------------------------------------ */

/**
 * Share text or a URL via the native share sheet.
 */
export async function shareContent(opts: { title?: string; text?: string; url?: string }): Promise<void> {
  try {
    await Share.share(opts);
  } catch { /* user cancelled or unavailable */ }
}

/* ------------------------------------------------------------------ */
/*  Status bar                                                         */
/* ------------------------------------------------------------------ */

export async function setDarkStatusBar(): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    await StatusBar.setStyle({ style: StatusBarStyle.Dark });
    await StatusBar.setBackgroundColor({ color: '#0f172a' }); // slate-900
  } catch { /* unavailable */ }
}

export async function hideStatusBar(): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    await StatusBar.hide();
  } catch { /* unavailable */ }
}

export async function showStatusBar(): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    await StatusBar.show();
  } catch { /* unavailable */ }
}

/* ------------------------------------------------------------------ */
/*  Toast                                                              */
/* ------------------------------------------------------------------ */

export async function showToast(text: string, duration: 'short' | 'long' = 'short'): Promise<void> {
  try {
    await Toast.show({ text, duration });
  } catch { /* unavailable */ }
}

/* ------------------------------------------------------------------ */
/*  Screen orientation                                                 */
/* ------------------------------------------------------------------ */

export async function lockPortrait(): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    await ScreenOrientation.lock({ orientation: 'portrait' });
  } catch { /* unavailable */ }
}

export async function unlockOrientation(): Promise<void> {
  if (!isNativePlatform()) return;
  try {
    await ScreenOrientation.unlock();
  } catch { /* unavailable */ }
}

/* ------------------------------------------------------------------ */
/*  Keep-awake (via native bridge evaluated JS — no extra plugin)      */
/* ------------------------------------------------------------------ */

let wakeLockSentinel: WakeLockSentinel | null = null;

/**
 * Keeps the screen on using the Web Wake Lock API (works in both native
 * and browser contexts on Android / Chrome).
 */
export async function keepAwakeOn(): Promise<boolean> {
  try {
    if ('wakeLock' in navigator) {
      wakeLockSentinel = await (navigator as any).wakeLock.request('screen');
      return true;
    }
  } catch { /* unavailable */ }
  return false;
}

export async function keepAwakeOff(): Promise<void> {
  try {
    if (wakeLockSentinel) {
      await wakeLockSentinel.release();
      wakeLockSentinel = null;
    }
  } catch { /* unavailable */ }
}

export function isKeepAwakeActive(): boolean {
  return wakeLockSentinel !== null && !wakeLockSentinel.released;
}
