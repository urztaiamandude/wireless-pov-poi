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
    
    # Save patterns to file
    output_dir = REPO_ROOT / "scripts" / "ai_agent" / "output"
    for pattern in result["patterns"]:
        filepath = generator.save_pattern_to_file(pattern, output_dir)
        pattern["saved_to"] = str(filepath)
    
    result["count"] = len(result["patterns"])
    result["timestamp"] = str(datetime.datetime.now())
    
    return result


if __name__ == "__main__":
    try:
        task_data = json.loads(sys.argv[1]) if len(sys.argv) > 1 else {}
    except json.JSONDecodeError:
        task_data = {}
    result = generate_patterns_task(task_data)
    print(json.dumps(result, indent=2))
