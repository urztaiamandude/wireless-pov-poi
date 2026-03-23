
import React, { useState, useEffect, useCallback } from 'react';
import {
  Camera, Vibrate, Wifi, WifiOff, Share2, Sun, SunDim,
  Smartphone, X, ChevronUp, ChevronDown, Lock, Unlock
} from 'lucide-react';
import {
  isNativePlatform,
  takePhoto,
  hapticImpact,
  hapticNotification,
  getNetworkStatus,
  onNetworkChange,
  shareContent,
  keepAwakeOn,
  keepAwakeOff,
  isKeepAwakeActive,
  showToast,
  lockPortrait,
  unlockOrientation,
  NetworkInfo,
  CapturedPhoto,
} from '../nativeFeatures';

interface NativeToolbarProps {
  /** Called when a photo is captured via the camera. */
  onPhotoCaptured?: (photo: CapturedPhoto) => void;
  /** The IP the device is expected to be on (for connection indicator). */
  deviceIp?: string;
}

const NativeToolbar: React.FC<NativeToolbarProps> = ({ onPhotoCaptured, deviceIp = '192.168.4.1' }) => {
  const [expanded, setExpanded] = useState(false);
  const [networkInfo, setNetworkInfo] = useState<NetworkInfo>({ connected: false, connectionType: 'unknown' });
  const [keepAwake, setKeepAwake] = useState(false);
  const [orientationLocked, setOrientationLocked] = useState(false);
  const [hapticsEnabled, setHapticsEnabled] = useState(true);

  // Only render on native platforms
  if (!isNativePlatform()) return null;

  // Subscribe to network changes
  useEffect(() => {
    getNetworkStatus().then(setNetworkInfo);
    const unsub = onNetworkChange(setNetworkInfo);
    return unsub;
  }, []);

  // Restore keep-awake state indicator on mount
  useEffect(() => {
    setKeepAwake(isKeepAwakeActive());
  }, []);

  const handleTakePhoto = useCallback(async () => {
    if (hapticsEnabled) await hapticImpact('light');
    const photo = await takePhoto();
    if (photo && onPhotoCaptured) {
      onPhotoCaptured(photo);
      if (hapticsEnabled) await hapticNotification('success');
      await showToast('Photo captured — open Image Lab to convert');
    }
  }, [onPhotoCaptured, hapticsEnabled]);

  const handleToggleKeepAwake = useCallback(async () => {
    if (keepAwake) {
      await keepAwakeOff();
      setKeepAwake(false);
      await showToast('Screen auto-off enabled');
    } else {
      const ok = await keepAwakeOn();
      setKeepAwake(ok);
      if (ok) await showToast('Screen will stay on during performance');
    }
    if (hapticsEnabled) await hapticImpact('light');
  }, [keepAwake, hapticsEnabled]);

  const handleToggleOrientation = useCallback(async () => {
    if (orientationLocked) {
      await unlockOrientation();
      setOrientationLocked(false);
      await showToast('Rotation unlocked');
    } else {
      await lockPortrait();
      setOrientationLocked(true);
      await showToast('Locked to portrait');
    }
    if (hapticsEnabled) await hapticImpact('light');
  }, [orientationLocked, hapticsEnabled]);

  const handleShare = useCallback(async () => {
    if (hapticsEnabled) await hapticImpact('light');
    await shareContent({
      title: 'Nebula POV Poi',
      text: 'Check out my POV poi — wireless LED persistence of vision!',
      url: 'https://github.com/urztaiamandude/wireless-pov-poi',
    });
  }, [hapticsEnabled]);

  const handleToggleHaptics = useCallback(async () => {
    const next = !hapticsEnabled;
    setHapticsEnabled(next);
    if (next) await hapticImpact('medium');
    await showToast(next ? 'Haptics on' : 'Haptics off');
  }, [hapticsEnabled]);

  const isWifi = networkInfo.connectionType === 'wifi';

  return (
    <div className="fixed bottom-4 right-4 z-50 flex flex-col items-end gap-2">
      {/* Expanded toolbar actions */}
      {expanded && (
        <div className="flex flex-col gap-2 animate-fadeIn">
          {/* Camera capture */}
          <ToolbarButton
            icon={<Camera size={20} />}
            label="Camera"
            onClick={handleTakePhoto}
            color="bg-pink-600 hover:bg-pink-500"
          />
          {/* Keep-awake toggle */}
          <ToolbarButton
            icon={keepAwake ? <Sun size={20} /> : <SunDim size={20} />}
            label={keepAwake ? 'Screen On' : 'Auto-Off'}
            onClick={handleToggleKeepAwake}
            color={keepAwake ? 'bg-yellow-600 hover:bg-yellow-500' : 'bg-slate-700 hover:bg-slate-600'}
          />
          {/* Orientation lock */}
          <ToolbarButton
            icon={orientationLocked ? <Lock size={20} /> : <Unlock size={20} />}
            label={orientationLocked ? 'Locked' : 'Rotate'}
            onClick={handleToggleOrientation}
            color={orientationLocked ? 'bg-indigo-600 hover:bg-indigo-500' : 'bg-slate-700 hover:bg-slate-600'}
          />
          {/* Haptics toggle */}
          <ToolbarButton
            icon={<Vibrate size={20} />}
            label={hapticsEnabled ? 'Haptics' : 'No Haptics'}
            onClick={handleToggleHaptics}
            color={hapticsEnabled ? 'bg-cyan-700 hover:bg-cyan-600' : 'bg-slate-700 hover:bg-slate-600'}
          />
          {/* Share */}
          <ToolbarButton
            icon={<Share2 size={20} />}
            label="Share"
            onClick={handleShare}
            color="bg-emerald-700 hover:bg-emerald-600"
          />
          {/* WiFi indicator (non-interactive) */}
          <div className={`flex items-center gap-2 px-3 py-2 rounded-full text-xs font-medium shadow-lg ${
            isWifi ? 'bg-green-800 text-green-200' : 'bg-red-900 text-red-300'
          }`}>
            {isWifi ? <Wifi size={16} /> : <WifiOff size={16} />}
            <span>{isWifi ? 'WiFi' : networkInfo.connectionType}</span>
          </div>
        </div>
      )}

      {/* Main FAB toggle */}
      <button
        onClick={() => { setExpanded(!expanded); if (hapticsEnabled) hapticImpact('light'); }}
        className="w-14 h-14 rounded-full bg-gradient-to-br from-cyan-500 to-purple-600 shadow-xl flex items-center justify-center text-white active:scale-95 transition-transform"
        aria-label={expanded ? 'Close native toolbar' : 'Open native toolbar'}
      >
        {expanded ? <X size={24} /> : <Smartphone size={24} />}
      </button>
    </div>
  );
};

/* ------------------------------------------------------------------ */

interface ToolbarButtonProps {
  icon: React.ReactNode;
  label: string;
  onClick: () => void;
  color: string;
}

const ToolbarButton: React.FC<ToolbarButtonProps> = ({ icon, label, onClick, color }) => (
  <button
    onClick={onClick}
    className={`flex items-center gap-2 px-4 py-2.5 rounded-full shadow-lg text-white text-sm font-medium transition-all active:scale-95 ${color}`}
  >
    {icon}
    <span>{label}</span>
  </button>
);

export default NativeToolbar;
