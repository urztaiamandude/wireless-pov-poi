#!/usr/bin/env python3
"""
Pattern Generation Agent - Creates new LED patterns for POV display
Generates patterns that can be added to the firmware
Free-tier compatible
"""

import sys
import json
import random
import datetime
import math
from pathlib import Path
from typing import Dict, List, Optional

REPO_ROOT = Path(__file__).parent.parent.parent


class PatternGenerator:
    """Generates new LED patterns for POV POI"""
    
    def __init__(self):
        # Pattern templates
        self.pattern_templates = {
            "rainbow_cycle": {
                "name": "Rainbow Cycle",
                "description": "Smooth rainbow color cycling across LEDs",
                "parameters": ["speed", "brightness"],
                "code_template": '''void pattern_rainbow_cycle(CRGB* leds, int num_leds, uint8_t hue_offset, uint8_t speed) {
    for (int i = 0; i < num_leds; i++) {
        uint8_t hue = (i * 256 / num_leds + hue_offset) % 256;
        leds[i] = CHSV(hue, 255, 255);
    }
    EVERY_N_MILLISECONDS(speed) { hue_offset += 5; }
}'''
            },
            "wave_sine": {
                "name": "Sine Wave",
                "description": "Animated sine wave pattern with brightness modulation",
                "parameters": ["speed", "amplitude", "frequency"],
                "code_template": '''void pattern_wave_sine(CRGB* leds, int num_leds, uint8_t time_offset, uint8_t speed) {
    for (int i = 0; i < num_leds; i++) {
        uint8_t brightness = sin8((i * 10 + time_offset) % 255);
        uint8_t hue = (i * 5 + time_offset) % 255;
        leds[i] = CHSV(hue, 255, brightness);
    }
    EVERY_N_MILLISECONDS(speed) { time_offset += 10; }
}'''
            },
            "comet_tail": {
                "name": "Comet Tail",
                "description": "Comet-like effect with trailing tail",
                "parameters": ["speed", "tail_length", "color"],
                "code_template": '''void pattern_comet_tail(CRGB* leds, int num_leds, uint8_t position, uint8_t speed) {
    static uint8_t head_position = 0;
    static const int tail_length = 8;
    
    // Fade all LEDs
    for (int i = 0; i < num_leds; i++) {
        leds[i].fadeToBlackBy(32);
    }
    
    // Draw comet head
    leds[head_position] = CRGB::Blue;
    leds[head_position].addToValue(100);
    
    // Draw comet tail
    for (int i = 1; i <= tail_length; i++) {
        int tail_pos = (head_position - i + num_leds) % num_leds;
        leds[tail_pos] = CRGB::Blue;
        leds[tail_pos].fadeToBlackBy(i * 32);
    }
    
    EVERY_N_MILLISECONDS(speed) { 
        head_position = (head_position + 1) % num_leds; 
    }
}'''
            },
            "fire_effect": {
                "name": "Fire Effect",
                "description": "Animated fire/flame effect",
                "parameters": ["cooling", "sparking", "speed"],
                "code_template": '''void pattern_fire_effect(CRGB* leds, int num_leds, uint8_t time_val) {
    static uint8_t heat[32];
    
    // Cool down
    for (int i = 0; i < num_leds; i++) {
        heat[i] = max(0, heat[i] - 2);
    }
    
    // Ignite new sparks
    if (random8() < 50) {
        int i = random8(num_leds);
        heat[i] = min(255, heat[i] + random8(100, 200));
    }
    
    // Heat transfer
    for (int i = num_leds - 1; i >= 2; i--) {
        heat[i] = (heat[i-1] + heat[i-2] + heat[i-2]) / 3;
    }
    
    // Map heat to colors
    for (int i = 0; i < num_leds; i++) {
        uint8_t t = heat[i] / 3;
        leds[i] = HeatColor(t);
    }
}'''
            },
            "plasma_wave": {
                "name": "Plasma Wave",
                "description": "Complex plasma-like color waves",
                "parameters": ["speed", "frequency", "amplitude"],
                "code_template": '''void pattern_plasma_wave(CRGB* leds, int num_leds, uint8_t time_val) {
    for (int i = 0; i < num_leds; i++) {
        uint8_t v = sin8(i * 5 + time_val);
        v += sin8(i * 10 + time_val * 2);
        v += sin8(i * 20 - time_val);
        v = v / 3 * 2;
        
        uint8_t hue = (i * 10 + time_val + v / 10) % 256;
        leds[i] = CHSV(hue, 200, v);
    }
    EVERY_N_MILLISECONDS(50) { time_val += 5; }
}'''
            },
            "pulse_beat": {
                "name": "Pulse Beat",
                "description": "Heartbeat-like pulsing effect",
                "parameters": ["min_brightness", "max_brightness", "speed"],
                "code_template": '''void pattern_pulse_beat(CRGB* leds, int num_leds, uint8_t beat_phase, uint8_t speed) {
    uint8_t brightness;
    
    // Double-beat pattern
    if (beat_phase < 30) {
        brightness = map(beat_phase, 0, 30, 50, 255);
    } else if (beat_phase < 40) {
        brightness = map(beat_phase, 30, 40, 255, 50);
    } else if (beat_phase < 70) {
        brightness = map(beat_phase, 40, 70, 50, 255);
    } else {
        brightness = map(beat_phase, 70, 100, 255, 50);
    }
    
    for (int i = 0; i < num_leds; i++) {
        uint8_t hue = (i * 256 / num_leds) % 256;
        leds[i] = CHSV(hue, 255, brightness);
    }
    
    EVERY_N_MILLISECONDS(speed) {
        beat_phase = (beat_phase + 5) % 100;
    }
}'''
            },
            "noise_shimmer": {
                "name": "Noise Shimmer",
                "description": "Perlin noise-based shimmer effect",
                "parameters": ["scale", "speed", "color_shift"],
                "code_template": '''void pattern_noise_shimmer(CRGB* leds, int num_leds, uint8_t time_val) {
    for (int i = 0; i < num_leds; i++) {
        uint16_t noise = inoise8(i * 30, time_val * 5);
        uint8_t hue = (noise + time_val * 2) % 256;
        uint8_t brightness = map(noise, 0, 255, 50, 255);
        leds[i] = CHSV(hue, 200, brightness);
    }
    EVERY_N_MILLISECONDS(50) { time_val++; }
}'''
            },
            "spiral_trace": {
                "name": "Spiral Trace",
                "description": "Spiral pattern tracing across the POV display",
                "parameters": ["speed", "width", "direction"],
                "code_template": '''void pattern_spiral_trace(CRGB* leds, int num_leds, uint8_t time_val, uint8_t speed) {
    static uint8_t head = 0;
    static uint8_t direction = 1;
    
    // Fade all LEDs
    for (int i = 0; i < num_leds; i++) {
        leds[i].fadeToBlackBy(64);
    }
    
    // Draw spiral
    for (int i = 0; i < 5; i++) {
        int pos = (head + i * direction + num_leds) % num_leds;
        uint8_t hue = (time_val + i * 30) % 256;
        leds[pos] = CHSV(hue, 255, 255);
    }
    
    EVERY_N_MILLISECONDS(speed) {
        head = (head + direction + num_leds) % num_leds;
        if (head == 0 || head == num_leds - 1) {
            direction *= -1;  // Reverse direction at ends
        }
    }
}'''
            }
        }
        
    def list_patterns(self) -> List[Dict]:
        """List all available pattern templates"""
        return [
            {
                "id": key,
                "name": template["name"],
                "description": template["description"],
                "parameters": template["parameters"]
            }
            for key, template in self.pattern_templates.items()
        ]
    
    def generate_pattern(self, pattern_id: str) -> Optional[Dict]:
        """Generate a specific pattern"""
        if pattern_id not in self.pattern_templates:
            return None
            
        template = self.pattern_templates[pattern_id]
        return {
            "id": pattern_id,
            "name": template["name"],
            "description": template["description"],
            "parameters": template["parameters"],
            "code": template["code_template"],
            "implementation_notes": self._get_implementation_notes(pattern_id)
        }
    
    def generate_random_patterns(self, count: int = 3) -> List[Dict]:
        """Generate random patterns"""
        pattern_ids = list(self.pattern_templates.keys())
        selected = random.sample(pattern_ids, min(count, len(pattern_ids)))
        return [self.generate_pattern(pid) for pid in selected]
    
    def _get_implementation_notes(self, pattern_id: str) -> str:
        """Get implementation notes for a pattern"""
        notes = {
            "rainbow_cycle": "Requires FastLED library with HSV color support. Adjust num_leds for your strip.",
            "wave_sine": "Uses sin8() for smooth brightness modulation. Good for calm animations.",
            "comet_tail": "Classic comet effect. Adjust tail_length for longer/shorter trails.",
            "fire_effect": "Based on fire algorithm. Cooling and sparking parameters affect flame behavior.",
            "plasma_wave": "Complex multi-sine pattern. May be computationally intensive.",
            "pulse_beat": "Simulates heartbeat rhythm. Good for rhythmic synchronization.",
            "noise_shimmer": "Requires Perlin noise function (inoise8 from FastLED).",
            "spiral_trace": "Bidirectional spiral pattern. Good for dynamic motion effects."
        }
        return notes.get(pattern_id, "Standard pattern implementation")
    
    def export_for_firmware(self, pattern: Dict) -> str:
        """Export pattern code ready for firmware integration"""
        code = pattern.get("code", "")
        return f'''// Pattern: {pattern['name']}
// Description: {pattern['description']}
// Generated by AI Pattern Agent

{code}

// Usage:
// void loop() {{
//     pattern_{pattern['id']}(leds, NUM_LEDS, hue_offset, speed);
// }}
'''

    def save_pattern_to_file(self, pattern: Dict, output_dir: Path) -> Path:
        """Save pattern to a file in the patterns directory"""
        patterns_dir = output_dir / "generated_patterns"
        patterns_dir.mkdir(parents=True, exist_ok=True)
        
        filename = f"pattern_{pattern['id']}.h"
        filepath = patterns_dir / filename
        
        content = self.export_for_firmware(pattern)
        filepath.write_text(content)
        
        return filepath


def generate_patterns_task(task_data: Dict) -> Dict:
    """Main pattern generation function"""
    generator = PatternGenerator()
    
    result = {
        "task": task_data.get("name", "pattern_generation"),
        "patterns": [],
        "all_patterns": generator.list_patterns()
    }
    
    # Generate specified number of patterns
    num_patterns = task_data.get("num_patterns", 3)
    pattern_ids = task_data.get("pattern_ids", [])
    
    if pattern_ids:
        # Generate specific patterns
        for pid in pattern_ids:
            pattern = generator.generate_pattern(pid)
            if pattern:
                result["patterns"].append(pattern)
    else:
        # Generate random patterns
        result["patterns"] = generator.generate_random_patterns(num_patterns)
    
    # Save patterns to files (both .h and .json)
    output_dir = REPO_ROOT / "scripts" / "ai_agent" / "output"
    patterns_dir = output_dir / "generated_patterns"
    patterns_dir.mkdir(parents=True, exist_ok=True)
    
    for pattern in result["patterns"]:
        # Save as header file
        h_filepath = generator.save_pattern_to_file(pattern, output_dir)
        pattern["saved_to_h"] = str(h_filepath)
        
        # Also save as JSON with pattern data
        json_data = {
            "id": pattern["id"],
            "name": pattern["name"],
            "description": pattern["description"],
            "parameters": pattern["parameters"],
            "implementation_notes": pattern.get("implementation_notes", ""),
            "pattern_data": generate_pattern_data(pattern["id"]),
            "code_snippet": pattern["code"]
        }
        
        json_filepath = patterns_dir / f"pattern_{pattern['id']}.json"
        with open(json_filepath, "w") as f:
            json.dump(json_data, f, indent=2)
        pattern["saved_to_json"] = str(json_filepath)
    
    result["count"] = len(result["patterns"])
    result["timestamp"] = str(datetime.datetime.now())
    result["output_directory"] = str(patterns_dir)
    
    return result


def generate_pattern_data(pattern_id: str) -> Dict:
    """Generate actual pattern data for a given pattern type"""
    import math
    
    if pattern_id == "rainbow_cycle":
        return {
            "type": "rainbow_cycle",
            "frames": 60,
            "speed": 50,
            "colors": [
                {"hue": i * 256 // 31, "saturation": 255, "brightness": 255}
                for i in range(31)
            ]
        }
    
    elif pattern_id == "wave_sine" or pattern_id == "sine_wave":
        return {
            "type": "sine_wave",
            "amplitude": 128,
            "frequency": 2,
            "speed": 40,
            "waveform": [
                int(128 + 127 * math.sin(2 * math.pi * i / 31))
                for i in range(31)
            ]
        }
    
    elif pattern_id == "comet_tail":
        return {
            "type": "comet_tail",
            "tail_length": 8,
            "speed": 60,
            "color": {"r": 0, "g": 100, "b": 255},
            "fade_rate": 32
        }
    
    elif pattern_id == "fire_effect":
        return {
            "type": "fire_effect",
            "cooling": 55,
            "sparking": 120,
            "speed": 30,
            "color_palette": "heat",
            "heat_map": [random.randint(0, 255) for _ in range(32)]
        }
    
    elif pattern_id == "plasma_wave":
        return {
            "type": "plasma_wave",
            "speed": 50,
            "complexity": 3,
            "color_shift": 10,
            "wave_functions": [
                {"amplitude": 5, "frequency": i+1, "phase": i*30}
                for i in range(3)
            ]
        }
    
    elif pattern_id == "pulse_beat":
        return {
            "type": "pulse_beat",
            "bpm": 120,
            "min_brightness": 50,
            "max_brightness": 255,
            "beat_pattern": [30, 40, 70, 100],  # Phase thresholds
            "double_beat": True
        }
    
    elif pattern_id == "noise_shimmer":
        return {
            "type": "noise_shimmer",
            "scale": 30,
            "speed": 50,
            "color_shift": 2,
            "noise_octaves": 2,
            "min_brightness": 50,
            "max_brightness": 255
        }
    
    elif pattern_id == "spiral_trace":
        return {
            "type": "spiral_trace",
            "speed": 80,
            "width": 5,
            "direction": 1,
            "bidirectional": True,
            "color_cycle": True
        }
    
    else:
        return {
            "type": pattern_id,
            "generic": True
        }


if __name__ == "__main__":
    try:
        task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    except json.JSONDecodeError:
        task_data = {}
    result = generate_patterns_task(task_data)
    print(json.dumps(result, indent=2))
