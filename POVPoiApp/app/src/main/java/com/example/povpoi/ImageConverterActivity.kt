package com.example.povpoi

import android.Manifest
import android.content.ContentValues
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.*
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.MediaStore
import android.widget.*
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.content.FileProvider
import androidx.lifecycle.lifecycleScope
import com.google.android.material.slider.Slider
import com.google.android.material.switchmaterial.SwitchMaterial
import kotlinx.coroutines.launch
import java.io.File
import java.io.OutputStream

/**
 * Image Converter Activity for POV POI System
 * Provides dedicated UI for converting images to POV-compatible format
 */
class ImageConverterActivity : AppCompatActivity() {
    
    private lateinit var imgOriginal: ImageView
    private lateinit var imgConverted: ImageView
    private lateinit var btnGallery: Button
    private lateinit var btnCamera: Button
    private lateinit var btnConvert: Button
    private lateinit var btnSave: Button
    private lateinit var btnUpload: Button
    private lateinit var sliderWidth: Slider
    private lateinit var sliderMaxHeight: Slider
    private lateinit var switchContrast: SwitchMaterial
    private lateinit var tvWidth: TextView
    private lateinit var tvMaxHeight: TextView
    private lateinit var tvStatus: TextView
    
    private var originalBitmap: Bitmap? = null
    private var convertedBitmap: Bitmap? = null
    private val povPoiAPI = POVPoiAPI()
    
    private var photoUri: Uri? = null
    
    // Image picker from gallery
    private val pickImageLauncher = registerForActivityResult(
        ActivityResultContracts.GetContent()
    ) { uri ->
        uri?.let { loadImage(it) }
    }
    
    // Take photo with camera
    private val takePictureLauncher = registerForActivityResult(
        ActivityResultContracts.TakePicture()
    ) { success ->
        if (success) {
            photoUri?.let { loadImage(it) }
        }
    }
    
    // Request camera permission
    private val requestCameraPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            openCamera()
        } else {
            Toast.makeText(this, "Camera permission required", Toast.LENGTH_SHORT).show()
        }
    }
    
    // Request storage permission (for Android 10 and below)
    private val requestStoragePermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            saveToGallery()
        } else {
            Toast.makeText(this, "Storage permission required", Toast.LENGTH_SHORT).show()
        }
    }
    
    // Request read media images permission (for Android 13+)
    private val requestReadMediaImagesLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            pickImageLauncher.launch("image/*")
        } else {
            Toast.makeText(this, "Permission required to access images", Toast.LENGTH_SHORT).show()
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_image_converter)
        
        // Setup toolbar
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.title = "Image Converter"
        
        initializeViews()
        setupListeners()
        
        updateStatus("Select an image to begin")
    }
    
    private fun initializeViews() {
        imgOriginal = findViewById(R.id.imgOriginal)
        imgConverted = findViewById(R.id.imgConverted)
        btnGallery = findViewById(R.id.btnGallery)
        btnCamera = findViewById(R.id.btnCamera)
        btnConvert = findViewById(R.id.btnConvert)
        btnSave = findViewById(R.id.btnSave)
        btnUpload = findViewById(R.id.btnUpload)
        sliderWidth = findViewById(R.id.sliderWidth)
        sliderMaxHeight = findViewById(R.id.sliderMaxHeight)
        switchContrast = findViewById(R.id.switchContrast)
        tvWidth = findViewById(R.id.tvWidth)
        tvMaxHeight = findViewById(R.id.tvMaxHeight)
        tvStatus = findViewById(R.id.tvStatus)
        
        // Initialize slider values
        tvWidth.text = "Width: ${sliderWidth.value.toInt()} pixels"
        tvMaxHeight.text = "Max Height: ${sliderMaxHeight.value.toInt()} pixels"
        
        // Initially disable action buttons
        btnConvert.isEnabled = false
        btnSave.isEnabled = false
        btnUpload.isEnabled = false
    }
    
    private fun setupListeners() {
        btnGallery.setOnClickListener {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                val permissionStatus = ContextCompat.checkSelfPermission(
                    this,
                    Manifest.permission.READ_MEDIA_IMAGES
                )
                if (permissionStatus != PackageManager.PERMISSION_GRANTED) {
                    requestReadMediaImagesLauncher.launch(Manifest.permission.READ_MEDIA_IMAGES)
                    return@setOnClickListener
                }
            }
            pickImageLauncher.launch("image/*")
        }
        
        btnCamera.setOnClickListener {
            if (checkCameraPermission()) {
                openCamera()
            } else {
                requestCameraPermissionLauncher.launch(Manifest.permission.CAMERA)
            }
        }
        
        btnConvert.setOnClickListener {
            convertImage()
        }
        
        btnSave.setOnClickListener {
            if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
                // Android 9 and below need storage permission
                if (checkStoragePermission()) {
                    saveToGallery()
                } else {
                    requestStoragePermissionLauncher.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                }
            } else {
                // Android 10+ use MediaStore (no permission needed)
                saveToGallery()
            }
        }
        
        btnUpload.setOnClickListener {
            uploadToDevice()
        }
        
        // Slider listeners
        sliderWidth.addOnChangeListener { _, value, _ ->
            tvWidth.text = "Width: ${value.toInt()} pixels"
        }
        
        sliderMaxHeight.addOnChangeListener { _, value, _ ->
            tvMaxHeight.text = "Max Height: ${value.toInt()} pixels"
        }
    }
    
    private fun checkCameraPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.CAMERA
        ) == PackageManager.PERMISSION_GRANTED
    }
    
    private fun checkStoragePermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
        ) == PackageManager.PERMISSION_GRANTED
    }
    
    private fun openCamera() {
        try {
            // Create temporary file for photo
            val photoFile = File.createTempFile(
                "POV_${System.currentTimeMillis()}",
                ".jpg",
                cacheDir
            )
            
            // Ensure the file was actually created
            if (!photoFile.exists()) {
                Toast.makeText(this, "Unable to create image file for camera", Toast.LENGTH_SHORT).show()
                return
            }
            
            val authority = "${applicationContext.packageName}.provider"
            
            // Get a content URI for the temporary file
            val uri = FileProvider.getUriForFile(
                this,
                authority,
                photoFile
            )
            
            // Basic validation of the URI before using it
            if (uri.scheme.isNullOrEmpty()) {
                Toast.makeText(this, "Invalid image URI for camera", Toast.LENGTH_SHORT).show()
                return
            }
            
            photoUri = uri
            takePictureLauncher.launch(photoUri)
        } catch (e: Exception) {
            Toast.makeText(this, "Unable to open camera: ${e.message}", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun loadImage(uri: Uri) {
        try {
            val inputStream = contentResolver.openInputStream(uri)
            originalBitmap = BitmapFactory.decodeStream(inputStream)
            inputStream?.close()
            
            if (originalBitmap != null) {
                imgOriginal.setImageBitmap(originalBitmap)
                btnConvert.isEnabled = true
                updateStatus("Image loaded. Click 'Convert Image' to process.")
            } else {
                Toast.makeText(this, "Failed to load image", Toast.LENGTH_SHORT).show()
            }
        } catch (e: Exception) {
            Toast.makeText(this, "Error loading image: ${e.message}", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun convertImage() {
        originalBitmap?.let { bitmap ->
            try {
                val targetWidth = sliderWidth.value.toInt()
                val maxHeight = sliderMaxHeight.value.toInt()
                val enhanceContrast = switchContrast.isChecked
                
                convertedBitmap = convertBitmapToPOVFormat(
                    bitmap,
                    targetWidth,
                    maxHeight,
                    enhanceContrast
                )
                
                updatePreviews()
                
                // Enable save and upload buttons
                btnSave.isEnabled = true
                btnUpload.isEnabled = true
                
                convertedBitmap?.let { converted ->
                    updateStatus("Converted: ${converted.width}x${converted.height} pixels")
                }
            } catch (e: Exception) {
                Toast.makeText(this, "Conversion failed: ${e.message}", Toast.LENGTH_SHORT).show()
            }
        }
    }
    
    private fun convertBitmapToPOVFormat(
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
        
        // Enhance contrast if requested
        return if (enhanceContrast) {
            enhanceContrast(resized, 2.0f)
        } else {
            resized
        }
    }
    
    private fun enhanceContrast(bitmap: Bitmap, factor: Float): Bitmap {
        val width = bitmap.width
        val height = bitmap.height

        // Always use ARGB_8888 to ensure consistent channel depth
        val result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)

        val pixels = IntArray(width * height)
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height)

        for (i in pixels.indices) {
            val color = pixels[i]
            val a = Color.alpha(color)
            var r = (Color.red(color) * factor).toInt()
            var g = (Color.green(color) * factor).toInt()
            var b = (Color.blue(color) * factor).toInt()

            // Clamp each channel to the valid RGB range [0, 255]
            r = r.coerceIn(0, 255)
            g = g.coerceIn(0, 255)
            b = b.coerceIn(0, 255)

            pixels[i] = Color.argb(a, r, g, b)
        }

        result.setPixels(pixels, 0, width, 0, 0, width, height)
        
        return result
    }
    
    private fun updatePreviews() {
        convertedBitmap?.let { converted ->
            // Create a scaled-up version for better visibility
            val scaleFactor = 10
            val displayBitmap = Bitmap.createScaledBitmap(
                converted,
                converted.width * scaleFactor,
                converted.height * scaleFactor,
                false // Use nearest neighbor for pixel art look
            )
            imgConverted.setImageBitmap(displayBitmap)
        }
    }
    
    private fun saveToGallery() {
        convertedBitmap?.let { bitmap ->
            try {
                val filename = "POV_${System.currentTimeMillis()}.png"
                val fos: OutputStream?
                
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    // Android 10+ use MediaStore
                    val resolver = contentResolver
                    val contentValues = ContentValues().apply {
                        put(MediaStore.MediaColumns.DISPLAY_NAME, filename)
                        put(MediaStore.MediaColumns.MIME_TYPE, "image/png")
                        put(MediaStore.MediaColumns.RELATIVE_PATH, "Pictures/POV_POI")
                    }
                    val imageUri = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)
                    fos = imageUri?.let { resolver.openOutputStream(it) }
                } else {
                    // Android 9 and below
                    val imagesDir = android.os.Environment.getExternalStoragePublicDirectory(
                        android.os.Environment.DIRECTORY_PICTURES
                    )
                    val image = File(imagesDir, filename)
                    fos = java.io.FileOutputStream(image)
                }
                
                fos?.use {
                    bitmap.compress(Bitmap.CompressFormat.PNG, 100, it)
                    Toast.makeText(this, "Image saved to gallery", Toast.LENGTH_SHORT).show()
                    updateStatus("Saved: $filename")
                }
            } catch (e: Exception) {
                Toast.makeText(this, "Failed to save: ${e.message}", Toast.LENGTH_SHORT).show()
            }
        }
    }
    
    private fun uploadToDevice() {
        convertedBitmap?.let { bitmap ->
            lifecycleScope.launch {
                try {
                    updateStatus("Uploading...")
                    btnUpload.isEnabled = false
                    
                    val success = povPoiAPI.uploadImage(bitmap)
                    
                    if (success) {
                        Toast.makeText(
                            this@ImageConverterActivity,
                            "Image uploaded successfully!",
                            Toast.LENGTH_SHORT
                        ).show()
                        updateStatus("Upload successful!")
                    } else {
                        Toast.makeText(
                            this@ImageConverterActivity,
                            "Upload failed",
                            Toast.LENGTH_SHORT
                        ).show()
                        updateStatus("Upload failed")
                    }
                } catch (e: Exception) {
                    Toast.makeText(
                        this@ImageConverterActivity,
                        "Upload error: ${e.message}",
                        Toast.LENGTH_SHORT
                    ).show()
                    updateStatus("Upload error: ${e.message}")
                } finally {
                    btnUpload.isEnabled = true
                }
            }
        }
    }
    
    private fun updateStatus(message: String) {
        tvStatus.text = message
    }
    
    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }
    
    override fun onDestroy() {
        super.onDestroy()
        originalBitmap?.recycle()
        convertedBitmap?.recycle()
    }
}
