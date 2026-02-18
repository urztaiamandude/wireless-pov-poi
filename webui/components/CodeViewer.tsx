
import React, { useState } from 'react';
import { ArduinoSketch } from '../types';
import { Copy, Check } from 'lucide-react';

interface CodeViewerProps {
  sketch: ArduinoSketch;
}

const CodeViewer: React.FC<CodeViewerProps> = ({ sketch }) => {
  const [copied, setCopied] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(sketch.code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleDownload = () => {
    const blob = new Blob([sketch.code], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${sketch.title.replace(/\s+/g, '_')}.ino`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="space-y-6">
      <header className="flex flex-col md:flex-row md:items-end justify-between gap-4">
        <div>
          <h2 className="text-3xl font-bold text-white mb-2">{sketch.title}</h2>
          <p className="text-slate-400">{sketch.description}</p>
        </div>
        <div className="flex gap-2">
          <button
            onClick={handleDownload}
            className="flex items-center gap-2 bg-slate-800 hover:bg-slate-700 text-white px-4 py-2 rounded-lg transition-all"
          >
            Download
          </button>
          <button
            onClick={handleCopy}
            className="flex items-center gap-2 bg-slate-800 hover:bg-slate-700 text-white px-4 py-2 rounded-lg transition-all"
          >
            {copied ? <Check size={18} className="text-green-400" /> : <Copy size={18} />}
            {copied ? 'Copied!' : 'Copy Code'}
          </button>
        </div>
      </header>

      <div className="flex flex-wrap gap-2">
        {sketch.libraries.map(lib => (
          <span key={lib} className="px-3 py-1 bg-slate-800 border border-slate-700 rounded-full text-xs font-mono text-cyan-400">
            #include &lt;{lib}&gt;
          </span>
        ))}
      </div>

      <div className="relative group">
        <div className="absolute top-0 right-0 p-4 opacity-0 group-hover:opacity-100 transition-opacity">
          <span className="text-xs text-slate-500 font-mono uppercase tracking-widest">C++ Arduino</span>
        </div>
        <pre className="bg-slate-900 border border-slate-800 rounded-xl p-6 overflow-x-auto font-mono text-sm text-slate-300 leading-relaxed custom-scrollbar shadow-2xl">
          <code>{sketch.code}</code>
        </pre>
      </div>

      <div className="bg-amber-900/20 border border-amber-500/30 rounded-lg p-4">
        <p className="text-amber-200 text-sm">
          <strong>Note:</strong> Ensure you have the <span className="underline decoration-amber-500/50">ArduinoJson</span> library installed via the Arduino Library Manager for JSON serialization.
        </p>
      </div>
    </div>
  );
};

export default CodeViewer;
