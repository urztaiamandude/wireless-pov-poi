

import { GoogleGenerativeAI } from "@google/genai";

// NOTE: API key is intentionally empty in client build for security.
// To enable AI features, implement a backend proxy endpoint that keeps the API key server-side.
const apiKey = process.env.API_KEY || '';
const genAI = apiKey ? new GoogleGenerativeAI(apiKey) : null;

export async function getExpansionAdvice(feature: string): Promise<string> {
  if (!genAI) {
    return "AI Assistant is not configured. To enable this feature, set up a backend proxy endpoint that securely handles Gemini API requests without exposing your API key in client-side code.";
  }

  const model = genAI.getGenerativeModel({ model: 'gemini-2.0-flash-exp' });
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
    const result = await model.generateContent(prompt);
    const response = await result.response;
    return response.text() || "Sorry, I couldn't generate a response at this time.";
  } catch (error) {
    console.error("Gemini API Error:", error);
    return "Error connecting to the expansion engine. Please check your configuration.";
  }
}
