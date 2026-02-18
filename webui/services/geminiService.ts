

import { GoogleGenAI } from "@google/genai";

const ai = new GoogleGenAI({ apiKey: process.env.API_KEY });

export async function getExpansionAdvice(feature: string): Promise<string> {
  const model = 'gemini-2.5-pro-preview-06-05';
  const prompt = `
    I am an embedded systems developer working on a POV (Persistence of Vision) system using a Teensy 4.1 and ESP32-S3.
    I want to add the following feature: ${feature}.

    Please provide:
    1. A brief high-level explanation of how to integrate this hardware.
    2. Which libraries are recommended (e.g., MPU6050 for gyro, FFT for audio).
    3. A snippet of C++ code for the Teensy 4.1 that would implement the core logic for this feature.

    Keep the tone professional and expert.
  `;

  try {
    const response = await ai.models.generateContent({
      model,
      contents: prompt,
    });
    return response.text || "Sorry, I couldn't generate a response at this time.";
  } catch (error) {
    console.error("Gemini API Error:", error);
    return "Error connecting to the expansion engine. Please check your configuration.";
  }
}
