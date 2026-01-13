package com.example.povpoi

import android.content.Intent
import android.os.Bundle
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.google.android.material.button.MaterialButton
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay

/**
 * Main Activity for Nebula Poi Controller App
 * Provides UI for controlling the Nebula Poi
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var api: POVPoiAPI
    
    // UI Components
    private lateinit var statusText: TextView
    private lateinit var modeText: TextView
    private lateinit var brightnessSeekBar: SeekBar
    private lateinit var brightnessValue: TextView
    private lateinit var framerateSeekBar: SeekBar
    private lateinit var framerateValue: TextView
    
    // Pattern buttons
    private lateinit var rainbowBtn: MaterialButton
    private lateinit var waveBtn: MaterialButton
    private lateinit var gradientBtn: MaterialButton
    private lateinit var sparkleBtn: MaterialButton
    
    // Action buttons
    private lateinit var imageConverterBtn: MaterialButton
    private lateinit var refreshBtn: MaterialButton
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        // Initialize API
        api = POVPoiAPI()
        
        // Initialize UI components
        initializeViews()
        setupListeners()
        
        // Start status updates
        startStatusUpdates()
    }
    
    private fun initializeViews() {
        statusText = findViewById(R.id.statusText)
        modeText = findViewById(R.id.modeText)
        brightnessSeekBar = findViewById(R.id.brightnessSeekBar)
        brightnessValue = findViewById(R.id.brightnessValue)
        framerateSeekBar = findViewById(R.id.framerateSeekBar)
        framerateValue = findViewById(R.id.framerateValue)
        
        rainbowBtn = findViewById(R.id.rainbowButton)
        waveBtn = findViewById(R.id.waveButton)
        gradientBtn = findViewById(R.id.gradientButton)
        sparkleBtn = findViewById(R.id.sparkleButton)
        imageConverterBtn = findViewById(R.id.imageConverterButton)
        refreshBtn = findViewById(R.id.refreshButton)
        
        // Setup seekbars
        brightnessSeekBar.progress = 128
        brightnessValue.text = "128"
        
        framerateSeekBar.progress = 50
        framerateValue.text = "60 FPS"
    }
    
    private fun setupListeners() {
        // Brightness control
        brightnessSeekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                brightnessValue.text = progress.toString()
                if (fromUser) {
                    setBrightness(progress)
                }
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
        
        // Framerate control
        framerateSeekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val fps = progress + 10
                framerateValue.text = "$fps FPS"
                if (fromUser) {
                    setFrameRate(fps)
                }
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
        
        // Pattern buttons
        rainbowBtn.setOnClickListener { setPattern(POVPoiAPI.PATTERN_RAINBOW) }
        waveBtn.setOnClickListener { setPattern(POVPoiAPI.PATTERN_WAVE) }
        gradientBtn.setOnClickListener { setPattern(POVPoiAPI.PATTERN_GRADIENT) }
        sparkleBtn.setOnClickListener { setPattern(POVPoiAPI.PATTERN_SPARKLE) }
        
        // Image converter button
        imageConverterBtn.setOnClickListener {
            startActivity(Intent(this, ImageConverterActivity::class.java))
        }
        
        // Refresh button
        refreshBtn.setOnClickListener {
            updateStatus()
            Toast.makeText(this, "Status refreshed", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun startStatusUpdates() {
        lifecycleScope.launch {
            while (true) {
                updateStatus()
                delay(2000) // Update every 2 seconds
            }
        }
    }
    
    private fun updateStatus() {
        lifecycleScope.launch {
            try {
                val status = api.getStatus()
                statusText.text = if (status.connected) 
                    getString(R.string.status_connected) 
                else 
                    getString(R.string.status_disconnected)
                    
                statusText.setTextColor(
                    if (status.connected) 
                        getColor(R.color.status_connected)
                    else 
                        getColor(R.color.status_disconnected)
                )
                
                val modeStr = when(status.mode) {
                    0 -> getString(R.string.mode_idle)
                    1 -> getString(R.string.mode_image)
                    2 -> getString(R.string.mode_pattern)
                    3 -> getString(R.string.mode_sequence)
                    4 -> getString(R.string.mode_live)
                    else -> "Mode: Unknown"
                }
                modeText.text = modeStr
            } catch (e: Exception) {
                statusText.text = getString(R.string.status_disconnected)
                statusText.setTextColor(getColor(R.color.status_disconnected))
                modeText.text = getString(R.string.connection_failed)
            }
        }
    }
    
    private fun setMode(mode: Int) {
        lifecycleScope.launch {
            try {
                api.setMode(mode, 0)
                Toast.makeText(this@MainActivity, "Mode changed", Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                Toast.makeText(this@MainActivity, "Failed to change mode", Toast.LENGTH_SHORT).show()
            }
        }
    }
    
    private fun setBrightness(brightness: Int) {
        lifecycleScope.launch {
            try {
                api.setBrightness(brightness)
            } catch (e: Exception) {
                // Silent fail for real-time updates
            }
        }
    }
    
    private fun setFrameRate(framerate: Int) {
        lifecycleScope.launch {
            try {
                api.setFrameRate(framerate)
            } catch (e: Exception) {
                // Silent fail for real-time updates
            }
        }
    }
    
    private fun setPattern(type: Int) {
        lifecycleScope.launch {
            try {
                val pattern = POVPoiAPI.PatternConfig(
                    index = 0,
                    type = type,
                    color1 = POVPoiAPI.RGBColor(255, 0, 0),
                    color2 = POVPoiAPI.RGBColor(0, 0, 255),
                    speed = 50
                )
                api.setPattern(pattern)
                
                // Switch to pattern mode if not already
                api.setMode(POVPoiAPI.MODE_PATTERN, 0)
                
                val patternName = when(type) {
                    POVPoiAPI.PATTERN_RAINBOW -> "Rainbow"
                    POVPoiAPI.PATTERN_WAVE -> "Wave"
                    POVPoiAPI.PATTERN_GRADIENT -> "Gradient"
                    POVPoiAPI.PATTERN_SPARKLE -> "Sparkle"
                    else -> "Unknown"
                }
                Toast.makeText(this@MainActivity, "$patternName pattern activated", Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                Toast.makeText(this@MainActivity, "Failed to set pattern", Toast.LENGTH_SHORT).show()
            }
        }
    }
}
