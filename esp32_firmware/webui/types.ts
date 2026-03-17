
export enum ViewMode {
  DASHBOARD = 'DASHBOARD',
  CODE_ESP32 = 'CODE_ESP32',
  CODE_TEENSY = 'CODE_TEENSY',
  WIRING = 'WIRING',
  FIRMWARE = 'FIRMWARE',
  ADVANCED_SETTINGS = 'ADVANCED_SETTINGS',
  IMAGE_LAB = 'IMAGE_LAB'
}

export interface Device {
  id: string;
  name: string;
  ip: string;
  status: string;
  type: 'success' | 'error' | 'info' | 'idle';
  isPlaying: boolean;
  brightness: number;
}

export interface ControlState {
  playing: boolean;
  brightness: number;
  filename: string;
}

export interface ArduinoSketch {
  title: string;
  code: string;
  description: string;
  libraries: string[];
}

export type PowerMode = 'performance' | 'balanced' | 'powersave' | 'ultrasave';

export interface SequenceItem {
  id: string;
  name: string;
  dataUrl: string;
  blob?: Blob;
  duration: number; // ms
}
