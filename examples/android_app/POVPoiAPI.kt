package com.example.povpoi

import android.graphics.Bitmap
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONArray
import org.json.JSONObject
import java.io.ByteArrayOutputStream
import java.io.IOException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * POV POI API Client
 * Provides methods to communicate with the POV POI system via REST API
 */
class POVPoiAPI(private val baseUrl: String = "http://192.168.4.1") {
    
    private val client = OkHttpClient.Builder()
        .connectTimeout(10, java.util.concurrent.TimeUnit.SECONDS)
        .readTimeout(10, java.util.concurrent.TimeUnit.SECONDS)
        .writeTimeout(10, java.util.concurrent.TimeUnit.SECONDS)
        .build()
    
    private val jsonMediaType = "application/json; charset=utf-8".toMediaType()
    private val binaryMediaType = "application/octet-stream".toMediaType()
    
    /**
     * System Status Data Class
     */
    data class SystemStatus(
        val connected: Boolean,
        val mode: Int,
        val index: Int,
        val brightness: Int,
        val framerate: Int
    )
    
    /**
     * RGB Color Data Class
     */
    data class RGBColor(
        val r: Int,
        val g: Int,
        val b: Int
    )
    
    /**
     * Pattern Configuration Data Class
     */
    data class PatternConfig(
        val index: Int,
        val type: Int,
        val color1: RGBColor,
        val color2: RGBColor,
        val speed: Int
    )
    
    /**
     * Get system status
     * @return SystemStatus object with current state
     */
    suspend fun getStatus(): SystemStatus = withContext(Dispatchers.IO) {
        val request = Request.Builder()
            .url("$baseUrl/api/status")
            .get()
            .build()
        
        val response = client.newCall(request).execute()
        if (!response.isSuccessful) throw IOException("Unexpected code $response")
        
        val jsonData = response.body?.string() ?: throw IOException("Empty response")
        val json = JSONObject(jsonData)
        
        SystemStatus(
            connected = json.getBoolean("connected"),
            mode = json.getInt("mode"),
            index = json.getInt("index"),
            brightness = json.getInt("brightness"),
            framerate = json.getInt("framerate")
        )
    }
    
    /**
     * Set display mode
     * @param mode Display mode (0=idle, 1=image, 2=pattern, 3=sequence, 4=live)
     * @param index Content index to display
     * @return true if successful
     */
    suspend fun setMode(mode: Int, index: Int): Boolean = withContext(Dispatchers.IO) {
        val json = JSONObject().apply {
            put("mode", mode)
            put("index", index)
        }
        
        val request = Request.Builder()
            .url("$baseUrl/api/mode")
            .post(json.toString().toRequestBody(jsonMediaType))
            .build()
        
        val response = client.newCall(request).execute()
        response.isSuccessful
    }
    
    /**
     * Set LED brightness
     * @param brightness Brightness level (0-255)
     * @return true if successful
     */
    suspend fun setBrightness(brightness: Int): Boolean = withContext(Dispatchers.IO) {
        val json = JSONObject().apply {
            put("brightness", brightness.coerceIn(0, 255))
        }
        
        val request = Request.Builder()
            .url("$baseUrl/api/brightness")
            .post(json.toString().toRequestBody(jsonMediaType))
            .build()
        
        val response = client.newCall(request).execute()
        response.isSuccessful
    }
    
    /**
     * Set frame rate
     * @param framerate Frame rate in FPS (10-120)
     * @return true if successful
     */
    suspend fun setFrameRate(framerate: Int): Boolean = withContext(Dispatchers.IO) {
        val json = JSONObject().apply {
            put("framerate", framerate.coerceIn(10, 120))
        }
        
        val request = Request.Builder()
            .url("$baseUrl/api/framerate")
            .post(json.toString().toRequestBody(jsonMediaType))
            .build()
        
        val response = client.newCall(request).execute()
        response.isSuccessful
    }
    
    /**
     * Upload pattern configuration
     * @param pattern PatternConfig object with pattern settings
     * @return true if successful
     */
    suspend fun setPattern(pattern: PatternConfig): Boolean = withContext(Dispatchers.IO) {
        val json = JSONObject().apply {
            put("index", pattern.index)
            put("type", pattern.type)
            put("color1", JSONObject().apply {
                put("r", pattern.color1.r)
                put("g", pattern.color1.g)
                put("b", pattern.color1.b)
            })
            put("color2", JSONObject().apply {
                put("r", pattern.color2.r)
                put("g", pattern.color2.g)
                put("b", pattern.color2.b)
            })
            put("speed", pattern.speed)
        }
        
        val request = Request.Builder()
            .url("$baseUrl/api/pattern")
            .post(json.toString().toRequestBody(jsonMediaType))
            .build()
        
        val response = client.newCall(request).execute()
        response.isSuccessful
    }
    
    /**
     * Upload image for POV display
     * Automatically converts image to POV-compatible format (31 pixels wide)
     * @param bitmap Source bitmap image
     * @return true if successful
     */
    suspend fun uploadImage(bitmap: Bitmap): Boolean = withContext(Dispatchers.IO) {
        // Convert bitmap to POV format (31 pixels wide)
        val povData = convertBitmapToPOVFormat(bitmap)
        
        // Upload raw RGB data
        val request = Request.Builder()
            .url("$baseUrl/api/image")
            .post(povData.toRequestBody(binaryMediaType))
            .build()
        
        client.newCall(request).execute().use { response ->
            response.isSuccessful
        }
    }
    
    /**
     * Convert bitmap to POV-compatible format
     * Resizes to 31 pixels wide, maintains aspect ratio, max 64 pixels tall
     * @param bitmap Source bitmap
     * @return ByteArray of RGB data (width * height * 3 bytes)
     */
    private fun convertBitmapToPOVFormat(bitmap: Bitmap): ByteArray {
        val targetWidth = 31
        val aspectRatio = bitmap.height.toFloat() / bitmap.width.toFloat()
        var targetHeight = (targetWidth * aspectRatio).toInt()
        
        // Limit height to 64 pixels
        if (targetHeight > 64) targetHeight = 64
        if (targetHeight < 1) targetHeight = 1
        
        // Resize bitmap using nearest neighbor (crisp pixels)
        val resized = Bitmap.createScaledBitmap(bitmap, targetWidth, targetHeight, false)
        
        // Flip vertically so bottom of image is at LED 1 (closest to board)
        // and top of image is at LED 31 (farthest from board)
        val matrix = android.graphics.Matrix()
        matrix.preScale(1.0f, -1.0f)
        val flipped = Bitmap.createBitmap(resized, 0, 0, targetWidth, targetHeight, matrix, false)
        
        try {
            // Pre-allocate byte array for efficiency
            val rgbData = ByteArray(targetWidth * targetHeight * 3)
            val pixels = IntArray(targetWidth * targetHeight)
            flipped.getPixels(pixels, 0, targetWidth, 0, 0, targetWidth, targetHeight)
            
            // Convert to RGB bytes (remove alpha channel)
            var byteIndex = 0
            for (pixel in pixels) {
                rgbData[byteIndex++] = ((pixel shr 16) and 0xFF).toByte()  // R
                rgbData[byteIndex++] = ((pixel shr 8) and 0xFF).toByte()   // G
                rgbData[byteIndex++] = (pixel and 0xFF).toByte()           // B
            }
            
            return rgbData
        } finally {
            // Ensure bitmaps are recycled even if an exception occurs
            resized.recycle()
            flipped.recycle()
        }
    }
    
    /**
     * Convert bitmap to POV-compatible format with enhanced options
     * Resizes to specified width, maintains aspect ratio, max specified height
     * @param bitmap Source bitmap
     * @param targetWidth Target width in pixels (default 31)
     * @param maxHeight Maximum height in pixels (default 64)
     * @param enhanceContrast Apply contrast enhancement (default true)
     * @return Bitmap in POV format
     */
    fun convertBitmapToPOVFormatEnhanced(
        bitmap: Bitmap,
        targetWidth: Int = 31,
        maxHeight: Int = 64,
        enhanceContrast: Boolean = true
    ): Bitmap {
        // Calculate new dimensions
        val aspectRatio = bitmap.height.toFloat() / bitmap.width.toFloat()
        var targetHeight = (targetWidth * aspectRatio).toInt()
        
        if (targetHeight > maxHeight) targetHeight = maxHeight
        if (targetHeight < 1) targetHeight = 1
        
        // Resize with nearest neighbor (no filtering for crisp pixels)
        val resized = Bitmap.createScaledBitmap(
            bitmap, targetWidth, targetHeight, false
        )
        
        // Flip vertically so bottom of image is at LED 1 (closest to board)
        // and top of image is at LED 31 (farthest from board)
        val matrix = android.graphics.Matrix()
        matrix.preScale(1.0f, -1.0f)
        val flipped = Bitmap.createBitmap(resized, 0, 0, targetWidth, targetHeight, matrix, false)
        resized.recycle() // Clean up intermediate bitmap
        
        // Enhance contrast if requested
        return if (enhanceContrast) {
            enhanceContrast(flipped, 2.0f)
        } else {
            flipped
        }
    }
    
    /**
     * Enhance contrast of a bitmap
     * @param bitmap Source bitmap
     * @param factor Contrast factor (1.0 = no change, 2.0 = double contrast)
     * @return Bitmap with enhanced contrast
     */
    private fun enhanceContrast(bitmap: Bitmap, factor: Float): Bitmap {
        val width = bitmap.width
        val height = bitmap.height

        // Always use ARGB_8888 to ensure consistent channel depth
        val result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)

        val pixels = IntArray(width * height)
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height)

        for (i in pixels.indices) {
            val color = pixels[i]
            val a = android.graphics.Color.alpha(color)
            var r = (android.graphics.Color.red(color) * factor).toInt()
            var g = (android.graphics.Color.green(color) * factor).toInt()
            var b = (android.graphics.Color.blue(color) * factor).toInt()

            // Clamp each channel to the valid RGB range [0, 255]
            r = r.coerceIn(0, 255)
            g = g.coerceIn(0, 255)
            b = b.coerceIn(0, 255)

            pixels[i] = android.graphics.Color.argb(a, r, g, b)
        }

        result.setPixels(pixels, 0, width, 0, 0, width, height)
        
        return result
    }
    
    /**
     * Send live frame data
     * @param pixels List of 31 RGB colors
     * @return true if successful
     */
    suspend fun sendLiveFrame(pixels: List<RGBColor>): Boolean = withContext(Dispatchers.IO) {
        require(pixels.size == 31) { "Must provide exactly 31 pixel colors" }
        
        val pixelsArray = JSONArray()
        pixels.forEach { color ->
            pixelsArray.put(JSONObject().apply {
                put("r", color.r)
                put("g", color.g)
                put("b", color.b)
            })
        }
        
        val json = JSONObject().apply {
            put("pixels", pixelsArray)
        }
        
        val request = Request.Builder()
            .url("$baseUrl/api/live")
            .post(json.toString().toRequestBody(jsonMediaType))
            .build()
        
        val response = client.newCall(request).execute()
        response.isSuccessful
    }
    
    /**
     * Pattern Type Constants
     */
    companion object {
        const val PATTERN_RAINBOW = 0
        const val PATTERN_WAVE = 1
        const val PATTERN_GRADIENT = 2
        const val PATTERN_SPARKLE = 3
        
        const val MODE_IDLE = 0
        const val MODE_IMAGE = 1
        const val MODE_PATTERN = 2
        const val MODE_SEQUENCE = 3
        const val MODE_LIVE = 4
    }
}
