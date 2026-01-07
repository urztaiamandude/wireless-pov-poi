package com.example.povpoi

import android.os.Bundle
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay

/**
 * Main Activity for POV POI Controller App
 * Provides UI for controlling the POV POI system
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var api: POVPoiAPI
    
    // UI Components
    private lateinit var statusText: TextView
    private lateinit var modeSpinner: Spinner
    private lateinit var brightnessSeekBar: SeekBar
    private lateinit var brightnessValue: TextView
    private lateinit var framerateSeekBar: SeekBar
    private lateinit var framerateValue: TextView
    
    // Pattern buttons
    private lateinit var rainbowBtn: Button
    private lateinit var waveBtn: Button
    private lateinit var gradientBtn: Button
    private lateinit var sparkleBtn: Button
    
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
        modeSpinner = findViewById(R.id.modeSpinner)
        brightnessSeekBar = findViewById(R.id.brightnessSeekBar)
        brightnessValue = findViewById(R.id.brightnessValue)
        framerateSeekBar = findViewById(R.id.framerateSeekBar)
        framerateValue = findViewById(R.id.framerateValue)
        
        rainbowBtn = findViewById(R.id.rainbowBtn)
        waveBtn = findViewById(R.id.waveBtn)
        gradientBtn = findViewById(R.id.gradientBtn)
        sparkleBtn = findViewById(R.id.sparkleBtn)
        
        // Setup mode spinner
        val modes = arrayOf("Idle", "Image", "Pattern", "Sequence", "Live")
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, modes)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        modeSpinner.adapter = adapter
        modeSpinner.setSelection(2) // Default to Pattern mode
        
        // Setup seekbars
        brightnessSeekBar.max = 255
        brightnessSeekBar.progress = 128
        brightnessValue.text = "128"
        
        framerateSeekBar.max = 110
        framerateSeekBar.progress = 40
        framerateValue.text = "50 FPS"
    }
    
    private fun setupListeners() {
        // Mode selection
        modeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: android.view.View?, position: Int, id: Long) {
                setMode(position)
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
        
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
                val statusStr = buildString {
                    append("Status: ")
                    append(if (status.connected) "Connected ✓" else "Disconnected ✗")
                    append("\nMode: ")
                    append(when(status.mode) {
                        0 -> "Idle"
                        1 -> "Image"
                        2 -> "Pattern"
                        3 -> "Sequence"
                        4 -> "Live"
                        else -> "Unknown"
                    })
                }
                statusText.text = statusStr
                statusText.setTextColor(
                    if (status.connected) 
                        getColor(android.R.color.holo_green_dark)
                    else 
                        getColor(android.R.color.holo_red_dark)
                )
            } catch (e: Exception) {
                statusText.text = "Status: Connection Error"
                statusText.setTextColor(getColor(android.R.color.holo_red_dark))
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
                modeSpinner.setSelection(POVPoiAPI.MODE_PATTERN)
                
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
