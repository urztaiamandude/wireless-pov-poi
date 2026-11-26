
import React, { useState, useEffect, useCallback } from 'react';
import { ViewMode } from './types';
import Dashboard from './components/Dashboard';
import PatternPreview from './components/PatternPreview';
import RetroStrobe from './components/RetroStrobe';
import WiringGuide from './components/WiringGuide';
import AdvancedSettings from './components/AdvancedSettings';
import ImageLab from './components/ImageLab';
import NativeToolbar from './components/NativeToolbar';
import {
  LayoutDashboard,
  CircuitBoard,
  Wrench,
  Github,
  Settings2,
  ImageIcon,
  Sparkles,
  Zap
} from 'lucide-react';
import { isNativePlatform, setDarkStatusBar, CapturedPhoto } from './nativeFeatures';
import { loadDeviceHost } from './deviceAddress';

const App: React.FC = () => {
  const [view, setView] = useState<ViewMode>(ViewMode.DASHBOARD);
  const [globalPreviewUrl, setGlobalPreviewUrl] = useState<string | null>(null);
  const [ledCount, setLedCount] = useState<number>(32);
  const [hostReady, setHostReady] = useState(false);

  // Load the saved poi address before the first /api call, and style the status bar.
  useEffect(() => {
    if (isNativePlatform()) {
      setDarkStatusBar();
    }
    loadDeviceHost().finally(() => setHostReady(true));
  }, []);

  // Handler for photos captured via NativeToolbar camera
  const handlePhotoCaptured = useCallback((photo: CapturedPhoto) => {
    setGlobalPreviewUrl(photo.dataUrl);
    setView(ViewMode.IMAGE_LAB);
  }, []);

  const renderContent = (current: ViewMode) => {
    switch (current) {
      case ViewMode.DASHBOARD:
        return <Dashboard previewUrl={globalPreviewUrl} />;
      case ViewMode.PATTERN_PREVIEW:
        return <PatternPreview ledCount={ledCount} />;
      case ViewMode.RETRO_STROBE:
        return <RetroStrobe />;
      case ViewMode.IMAGE_LAB:
        return <ImageLab onPreviewUpdate={setGlobalPreviewUrl} initialPreview={globalPreviewUrl} ledCount={ledCount} setLedCount={setLedCount} />;
      case ViewMode.ADVANCED_SETTINGS:
        return <AdvancedSettings ledCount={ledCount} setLedCount={setLedCount} />;
      case ViewMode.WIRING:
        return <WiringGuide />;
      default: {
        const _exhaustive: never = current;
        return _exhaustive;
      }
    }
  };

  if (!hostReady) {
    return <div className="min-h-screen bg-slate-950" />;
  }

  return (
    <div className="flex flex-col lg:flex-row min-h-screen">
      {/* Sidebar Navigation */}
      <nav className="w-full lg:w-64 bg-slate-900 border-b lg:border-r border-slate-800 p-4 space-y-1 lg:fixed lg:h-full z-20 overflow-y-auto custom-scrollbar">
        <div className="flex items-center gap-2 px-2 py-4 mb-4">
          <CircuitBoard className="text-cyan-400 w-8 h-8" />
          <div>
            <h1 className="text-xl font-bold bg-gradient-to-r from-cyan-400 to-purple-400 bg-clip-text text-transparent">
              POV Architect
            </h1>
            <p className="text-[9px] text-slate-600 font-mono">Nebula POI • wireless-pov-poi</p>
          </div>
        </div>

        <NavItem
          active={view === ViewMode.DASHBOARD}
          onClick={() => setView(ViewMode.DASHBOARD)}
          icon={<LayoutDashboard size={20} />}
          label="UI Dashboard"
        />
        <NavItem
          active={view === ViewMode.PATTERN_PREVIEW}
          onClick={() => setView(ViewMode.PATTERN_PREVIEW)}
          icon={<Sparkles size={20} className="text-cyan-400" />}
          label="Pattern Preview"
        />
        <NavItem
          active={view === ViewMode.RETRO_STROBE}
          onClick={() => setView(ViewMode.RETRO_STROBE)}
          icon={<Zap size={20} className="text-orange-400" />}
          label="Retro Strobe"
        />
        <NavItem
          active={view === ViewMode.IMAGE_LAB}
          onClick={() => setView(ViewMode.IMAGE_LAB)}
          icon={<ImageIcon size={20} className="text-pink-400" />}
          label="Image Lab"
        />
        <NavItem
          active={view === ViewMode.ADVANCED_SETTINGS}
          onClick={() => setView(ViewMode.ADVANCED_SETTINGS)}
          icon={<Settings2 size={20} />}
          label="Display Config"
        />
        <div className="h-4" />
        <NavItem
          active={view === ViewMode.WIRING}
          onClick={() => setView(ViewMode.WIRING)}
          icon={<Wrench size={20} />}
          label="Wiring Guide"
        />

        <div className="pt-8 px-4 hidden lg:block">
          <a
            href="https://github.com/urztaiamandude/wireless-pov-poi"
            target="_blank"
            rel="noreferrer"
            className="flex items-center gap-2 text-xs text-slate-500 hover:text-slate-300 transition-colors"
          >
            <Github size={14} />
            wireless-pov-poi on GitHub
          </a>
        </div>
      </nav>

      {/* Main Content Area */}
      <main className="flex-1 lg:ml-64 p-4 md:p-8 bg-slate-950">
        <div className="max-w-5xl mx-auto animate-fadeIn">
          {renderContent(view)}
        </div>
      </main>

      {/* Native-only floating toolbar (camera, keep-awake, share, etc.) */}
      <NativeToolbar onPhotoCaptured={handlePhotoCaptured} />
    </div>
  );
};

const NavItem = React.memo<{ active: boolean, onClick: () => void, icon: React.ReactNode, label: string }>(({ active, onClick, icon, label }) => (
  <button
    onClick={onClick}
    className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 ${
      active
        ? 'bg-slate-800 text-white shadow-lg ring-1 ring-slate-700'
        : 'text-slate-400 hover:bg-slate-800/50 hover:text-slate-200'
    }`}
  >
    {icon}
    <span className="font-medium">{label}</span>
  </button>
));

NavItem.displayName = 'NavItem';

export default App;
