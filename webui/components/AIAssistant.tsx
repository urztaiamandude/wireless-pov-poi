
import React, { useState } from 'react';
import { getExpansionAdvice } from '../services/geminiService';
import { Sparkles, Send, Loader2, Music, Maximize } from 'lucide-react';

const AIAssistant: React.FC = () => {
  const [input, setInput] = useState('');
  const [result, setResult] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSuggest = async (feature: string) => {
    setLoading(true);
    const advice = await getExpansionAdvice(feature);
    setResult(advice);
    setLoading(false);
  };

  const handleCustomSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim()) return;
    setLoading(true);
    const advice = await getExpansionAdvice(input);
    setResult(advice);
    setLoading(false);
  };

  return (
    <div className="space-y-8">
      <header>
        <h2 className="text-3xl font-bold text-white mb-2 flex items-center gap-2">
          <Sparkles className="text-purple-400" /> AI System Architect
        </h2>
        <p className="text-slate-400">Expand your POV system with modular enhancements suggested by Gemini AI.</p>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        <div className="space-y-6">
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-6">
            <h3 className="text-white font-semibold mb-4">Quick Enhancements</h3>
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
              <button
                onClick={() => handleSuggest('Add an MPU6050 Gyroscopic Accelerometer for image stabilization and perspective correction')}
                className="flex items-center gap-2 p-3 bg-slate-800 hover:bg-slate-700 rounded-lg text-sm text-left transition-all"
              >
                <Maximize size={16} className="text-cyan-400 shrink-0" />
                Gyroscopic Stability
              </button>
              <button
                onClick={() => handleSuggest('Add an I2S Microphone (INMP441) for audio-reactive patterns using FFT')}
                className="flex items-center gap-2 p-3 bg-slate-800 hover:bg-slate-700 rounded-lg text-sm text-left transition-all"
              >
                <Music size={16} className="text-pink-400 shrink-0" />
                Audio Reactivity
              </button>
            </div>
          </div>

          <form onSubmit={handleCustomSubmit} className="relative">
            <input
              type="text"
              value={input}
              onChange={(e) => setInput(e.target.value)}
              placeholder="Describe a feature (e.g., 'Add a Hall Effect sensor' or 'DMX control')..."
              className="w-full bg-slate-900 border border-slate-800 rounded-xl pl-6 pr-14 py-4 focus:ring-2 focus:ring-purple-500 focus:outline-none"
            />
            <button
              type="submit"
              disabled={loading}
              className="absolute right-3 top-1/2 -translate-y-1/2 p-2 bg-purple-600 hover:bg-purple-500 rounded-lg text-white disabled:opacity-50 transition-all"
            >
              {loading ? <Loader2 className="animate-spin" size={20} /> : <Send size={20} />}
            </button>
          </form>

          <div className="bg-blue-900/10 border border-blue-500/20 rounded-xl p-6">
            <h4 className="text-blue-400 font-bold mb-2">Expert Tip</h4>
            <p className="text-slate-400 text-sm leading-relaxed">
              When adding Gyro or Audio logic, the Teensy 4.1's 600MHz processor is more than capable.
              However, ensure your <code className="text-blue-300">FastLED.show()</code> frequency doesn't interfere with your sensor polling rates.
            </p>
          </div>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 min-h-[400px] flex flex-col">
          <div className="flex items-center justify-between mb-4 pb-4 border-b border-slate-800">
            <h3 className="text-white font-semibold">Implementation Advice</h3>
            {result && <button onClick={() => setResult('')} className="text-xs text-slate-500 hover:text-slate-300">Clear</button>}
          </div>

          <div className="flex-1 overflow-y-auto custom-scrollbar">
            {loading ? (
              <div className="flex flex-col items-center justify-center h-full space-y-4">
                <Loader2 className="animate-spin text-purple-400" size={32} />
                <p className="text-slate-500 animate-pulse">Analyzing system architecture...</p>
              </div>
            ) : result ? (
              <div className="prose prose-invert prose-sm max-w-none prose-pre:bg-black prose-pre:border prose-pre:border-slate-800">
                <div className="text-slate-300 whitespace-pre-wrap font-sans">
                  {result}
                </div>
              </div>
            ) : (
              <div className="flex flex-col items-center justify-center h-full text-slate-600">
                <Sparkles size={48} className="mb-4 opacity-20" />
                <p>Select a suggestion or type your own to get started.</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};

export default AIAssistant;
