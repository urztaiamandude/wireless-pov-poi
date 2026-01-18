#!/usr/bin/env python3
"""
Nebula Poi Image Converter - GUI Version
User-friendly interface for converting images to POV format
"""

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import os
import sys
from threading import Thread

# Configuration constants
MIN_IMAGE_WIDTH_FOR_SCALING = 50  # Minimum width to trigger upscaling in preview
CONTRAST_ENHANCEMENT_FACTOR = 2.0  # Factor for contrast enhancement

# Check for PIL/Pillow with helpful error handling
try:
    from PIL import Image, ImageTk, ImageEnhance
except ImportError:
    # Show GUI error dialog with installation instructions
    root = tk.Tk()
    root.withdraw()
    messagebox.showerror(
        "Missing Dependency",
        "Pillow library is required.\n\n"
        "Install with:\n"
        "pip install Pillow\n\n"
        "Or:\n"
        "python -m pip install Pillow\n\n"
        "Or run the setup script:\n"
        "python install_dependencies.py"
    )
    sys.exit(1)

# Import the conversion function from the existing module
try:
    from image_converter import convert_image_for_pov
except ImportError:
    print("Error: Could not import image_converter module")
    print("Make sure image_converter.py is in the same directory")
    sys.exit(1)


class POVImageConverterGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Nebula Poi Image Converter")
        self.root.geometry("900x700")
        self.root.resizable(True, True)
        
        # Variables
        self.input_files = []
        self.current_file_index = 0
        self.original_image = None
        self.converted_image = None
        self.preview_scale = 10  # Scale factor for preview display
        self.original_aspect_ratio = 1.0  # Store original aspect ratio
        
        # Settings variables
        # NOTE: HEIGHT is FIXED at 31 (matching display LEDs), WIDTH is calculated
        self.height_var = tk.IntVar(value=31)  # Fixed: matches 31 display LEDs
        self.max_width_var = tk.IntVar(value=200)  # Max width limit
        self.contrast_var = tk.BooleanVar(value=True)
        self.aspect_ratio_lock_var = tk.BooleanVar(value=True)
        self.flip_horizontal_var = tk.BooleanVar(value=False)
        
        # Setup UI
        self.setup_ui()
        
        # Update status
        self.update_status("Ready. Select an image to begin.")
    
    def setup_ui(self):
        """Setup the user interface"""
        
        # Header
        header_frame = tk.Frame(self.root, bg="#2c3e50", height=60)
        header_frame.pack(fill=tk.X, side=tk.TOP)
        header_frame.pack_propagate(False)
        
        title_label = tk.Label(
            header_frame,
            text="Nebula Poi Image Converter",
            font=("Arial", 18, "bold"),
            bg="#2c3e50",
            fg="white"
        )
        title_label.pack(pady=15)
        
        # Main content area
        main_frame = tk.Frame(self.root, padx=20, pady=20)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Button frame
        button_frame = tk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(0, 15))
        
        self.select_btn = tk.Button(
            button_frame,
            text="Select Image",
            command=self.select_image,
            bg="#3498db",
            fg="white",
            font=("Arial", 10, "bold"),
            padx=20,
            pady=10,
            cursor="hand2"
        )
        self.select_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.select_multiple_btn = tk.Button(
            button_frame,
            text="Select Multiple",
            command=self.select_multiple_images,
            bg="#9b59b6",
            fg="white",
            font=("Arial", 10, "bold"),
            padx=20,
            pady=10,
            cursor="hand2"
        )
        self.select_multiple_btn.pack(side=tk.LEFT)
        
        # File info label
        self.file_info_label = tk.Label(
            button_frame,
            text="No file selected",
            font=("Arial", 9),
            fg="#7f8c8d"
        )
        self.file_info_label.pack(side=tk.LEFT, padx=20)
        
        # Preview frame
        preview_frame = tk.Frame(main_frame)
        preview_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 15))
        
        # Before preview
        before_frame = tk.LabelFrame(
            preview_frame,
            text="Before",
            font=("Arial", 10, "bold"),
            padx=10,
            pady=10
        )
        before_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10))
        
        self.before_canvas = tk.Canvas(
            before_frame,
            bg="#ecf0f1",
            highlightthickness=1,
            highlightbackground="#bdc3c7"
        )
        self.before_canvas.pack(fill=tk.BOTH, expand=True)
        
        self.before_label = tk.Label(
            before_frame,
            text="Original Image",
            font=("Arial", 9),
            fg="#7f8c8d"
        )
        self.before_label.pack(pady=(5, 0))
        
        # After preview
        after_frame = tk.LabelFrame(
            preview_frame,
            text="After",
            font=("Arial", 10, "bold"),
            padx=10,
            pady=10
        )
        after_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        self.after_canvas = tk.Canvas(
            after_frame,
            bg="#ecf0f1",
            highlightthickness=1,
            highlightbackground="#bdc3c7"
        )
        self.after_canvas.pack(fill=tk.BOTH, expand=True)
        
        self.after_label = tk.Label(
            after_frame,
            text="POV Format (31px high)",
            font=("Arial", 9),
            fg="#7f8c8d"
        )
        self.after_label.pack(pady=(5, 0))
        
        # Settings frame
        settings_frame = tk.LabelFrame(
            main_frame,
            text="Conversion Settings",
            font=("Arial", 10, "bold"),
            padx=15,
            pady=15
        )
        settings_frame.pack(fill=tk.X, pady=(0, 15))
        
        # Height setting (FIXED - matches display LEDs)
        height_frame = tk.Frame(settings_frame)
        height_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Label(
            height_frame,
            text="Height:",
            font=("Arial", 10),
            width=12,
            anchor="w"
        ).pack(side=tk.LEFT)
        
        height_spinbox = tk.Spinbox(
            height_frame,
            from_=1,
            to=100,
            textvariable=self.height_var,
            width=10,
            font=("Arial", 10),
            command=lambda: self.on_dimension_change('height')
        )
        height_spinbox.pack(side=tk.LEFT, padx=(0, 5))
        height_spinbox.bind('<KeyRelease>', lambda e: self.on_dimension_change('height'))
        
        tk.Label(
            height_frame,
            text="pixels (FIXED: 31 = display LEDs)",
            font=("Arial", 9),
            fg="#e74c3c"
        ).pack(side=tk.LEFT)
        
        # Max width setting (calculated based on aspect ratio)
        width_frame = tk.Frame(settings_frame)
        width_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Label(
            width_frame,
            text="Max Width:",
            font=("Arial", 10),
            width=12,
            anchor="w"
        ).pack(side=tk.LEFT)
        
        width_spinbox = tk.Spinbox(
            width_frame,
            from_=1,
            to=500,
            textvariable=self.max_width_var,
            width=10,
            font=("Arial", 10),
            command=lambda: self.on_dimension_change('width')
        )
        width_spinbox.pack(side=tk.LEFT, padx=(0, 5))
        width_spinbox.bind('<KeyRelease>', lambda e: self.on_dimension_change('width'))
        
        tk.Label(
            width_frame,
            text="pixels (calculated from aspect ratio)",
            font=("Arial", 9),
            fg="#7f8c8d"
        ).pack(side=tk.LEFT)
        
        # Contrast enhancement checkbox
        contrast_check = tk.Checkbutton(
            settings_frame,
            text="Enhance Contrast (recommended for better visibility)",
            variable=self.contrast_var,
            font=("Arial", 10),
            command=self.on_settings_change
        )
        contrast_check.pack(anchor="w")
        
        # Aspect ratio lock checkbox
        aspect_lock_check = tk.Checkbutton(
            settings_frame,
            text="Lock Aspect Ratio",
            variable=self.aspect_ratio_lock_var,
            font=("Arial", 10)
        )
        aspect_lock_check.pack(anchor="w", pady=(5, 0))
        
        # Flip options frame
        flip_frame = tk.Frame(settings_frame)
        flip_frame.pack(fill=tk.X, pady=(10, 0))
        
        tk.Label(
            flip_frame,
            text="Flip Options:",
            font=("Arial", 10, "bold")
        ).pack(anchor="w")
        
        flip_horizontal_check = tk.Checkbutton(
            flip_frame,
            text="Flip Horizontal",
            variable=self.flip_horizontal_var,
            font=("Arial", 10),
            command=self.on_settings_change
        )
        flip_horizontal_check.pack(anchor="w")
        
        # Info label about LED orientation
        tk.Label(
            flip_frame,
            text="(No vertical flip needed - LEDs map directly to image)",
            font=("Arial", 8),
            fg="#7f8c8d"
        ).pack(anchor="w")
        
        # Action buttons frame
        action_frame = tk.Frame(main_frame)
        action_frame.pack(fill=tk.X, pady=(0, 15))
        
        self.convert_btn = tk.Button(
            action_frame,
            text="Convert & Save",
            command=self.convert_and_save,
            bg="#27ae60",
            fg="white",
            font=("Arial", 11, "bold"),
            padx=30,
            pady=12,
            cursor="hand2",
            state=tk.DISABLED
        )
        self.convert_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.batch_convert_btn = tk.Button(
            action_frame,
            text="Batch Convert All",
            command=self.batch_convert_all,
            bg="#f39c12",
            fg="white",
            font=("Arial", 11, "bold"),
            padx=30,
            pady=12,
            cursor="hand2",
            state=tk.DISABLED
        )
        self.batch_convert_btn.pack(side=tk.LEFT)
        
        # Status bar
        status_frame = tk.Frame(self.root, bg="#34495e", height=30)
        status_frame.pack(fill=tk.X, side=tk.BOTTOM)
        status_frame.pack_propagate(False)
        
        self.status_label = tk.Label(
            status_frame,
            text="Ready",
            font=("Arial", 9),
            bg="#34495e",
            fg="white",
            anchor="w",
            padx=10
        )
        self.status_label.pack(fill=tk.BOTH)
        
        # Progress bar (initially hidden)
        self.progress = ttk.Progressbar(
            status_frame,
            mode='indeterminate'
        )
    
    def update_status(self, message):
        """Update status bar message"""
        self.status_label.config(text=message)
        self.root.update_idletasks()
    
    def select_image(self):
        """Open file dialog to select a single image"""
        filetypes = (
            ("Image files", "*.jpg *.jpeg *.png *.gif *.bmp"),
            ("All files", "*.*")
        )
        
        filename = filedialog.askopenfilename(
            title="Select an image",
            filetypes=filetypes
        )
        
        if filename:
            self.input_files = [filename]
            self.current_file_index = 0
            self.load_and_preview_image(filename)
            self.convert_btn.config(state=tk.NORMAL)
            self.batch_convert_btn.config(state=tk.DISABLED)
    
    def select_multiple_images(self):
        """Open file dialog to select multiple images"""
        filetypes = (
            ("Image files", "*.jpg *.jpeg *.png *.gif *.bmp"),
            ("All files", "*.*")
        )
        
        filenames = filedialog.askopenfilenames(
            title="Select images",
            filetypes=filetypes
        )
        
        if filenames:
            self.input_files = list(filenames)
            self.current_file_index = 0
            self.load_and_preview_image(self.input_files[0])
            self.convert_btn.config(state=tk.NORMAL)
            if len(self.input_files) > 1:
                self.batch_convert_btn.config(state=tk.NORMAL)
            self.update_status(f"Loaded {len(self.input_files)} files. Showing first image.")
    
    def load_and_preview_image(self, filepath):
        """Load image and show before/after previews"""
        try:
            # Update file info
            filename = os.path.basename(filepath)
            self.file_info_label.config(text=f"File: {filename}")
            
            # Load original image
            self.original_image = Image.open(filepath)
            orig_width, orig_height = self.original_image.size
            
            # Store original aspect ratio for aspect ratio lock
            self.original_aspect_ratio = orig_height / orig_width
            
            # Update dimensions if aspect ratio lock is on
            if self.aspect_ratio_lock_var.get():
                current_width = self.width_var.get()
                new_height = int(round(current_width * self.original_aspect_ratio))
                new_height = max(1, min(200, new_height))  # Clamp to valid range
                self.height_var.set(new_height)
            
            # Update before label
            self.before_label.config(
                text=f"Original: {orig_width}x{orig_height}px"
            )
            
            # Display original image (scaled for preview)
            self.display_image_on_canvas(
                self.before_canvas,
                self.original_image,
                scale_up=True
            )
            
            # Generate preview of converted image
            self.update_preview()
            
            self.update_status(f"Loaded: {filename} ({orig_width}x{orig_height}px)")
            
        except Exception as e:
            messagebox.showerror(
                "Error Loading Image",
                f"Failed to load image:\n{str(e)}"
            )
            self.update_status("Error loading image")
    
    def display_image_on_canvas(self, canvas, image, scale_up=False):
        """Display an image on a canvas, centered"""
        canvas.delete("all")
        
        # Get canvas size
        canvas.update_idletasks()
        canvas_width = canvas.winfo_width()
        canvas_height = canvas.winfo_height()
        
        if canvas_width <= 1:
            canvas_width = 300
        if canvas_height <= 1:
            canvas_height = 300
        
        # Calculate display size
        img_width, img_height = image.size
        
        if scale_up and img_width < MIN_IMAGE_WIDTH_FOR_SCALING:
            # Scale up small images for better visibility
            scale_factor = min(
                canvas_width // img_width,
                canvas_height // img_height,
                self.preview_scale
            )
            display_width = img_width * scale_factor
            display_height = img_height * scale_factor
            
            # Use NEAREST for crisp pixel scaling
            display_image = image.resize(
                (display_width, display_height),
                Image.NEAREST
            )
        else:
            # Scale down large images to fit canvas
            scale_factor = min(
                canvas_width / img_width,
                canvas_height / img_height
            )
            
            if scale_factor < 1:
                display_width = int(img_width * scale_factor)
                display_height = int(img_height * scale_factor)
                display_image = image.resize(
                    (display_width, display_height),
                    Image.LANCZOS
                )
            else:
                display_image = image
        
        # Convert to PhotoImage
        photo = ImageTk.PhotoImage(display_image)
        
        # Center image on canvas
        x = canvas_width // 2
        y = canvas_height // 2
        canvas.create_image(x, y, image=photo, anchor=tk.CENTER)
        
        # Keep a reference to prevent garbage collection
        canvas.image = photo
    
    def update_preview(self):
        """Generate and display preview of converted image"""
        if self.original_image is None:
            return
        
        try:
            # Convert image in memory
            img = self.original_image.copy()
            
            # Convert to RGB if necessary
            if img.mode != 'RGB':
                img = img.convert('RGB')
            
            # Get settings
            # HEIGHT is FIXED (matches display LEDs), WIDTH is calculated
            height = self.height_var.get()
            max_width = self.max_width_var.get()
            enhance_contrast = self.contrast_var.get()
            flip_horizontal = self.flip_horizontal_var.get()
            
            # Calculate new width maintaining aspect ratio
            # HEIGHT is fixed at 31 (display LEDs), WIDTH is calculated
            aspect_ratio = img.width / img.height
            new_width = int(height * aspect_ratio)
            
            # Limit width
            if new_width > max_width:
                new_width = max_width
            if new_width < 1:
                new_width = 1
            
            # Resize with nearest neighbor for crisp pixels
            img = img.resize((new_width, height), Image.NEAREST)
            
            # Apply horizontal flip if requested
            if flip_horizontal:
                img = img.transpose(Image.FLIP_LEFT_RIGHT)
            
            # No vertical flip needed - LED 1 (bottom) maps to image bottom,
            # LED 31 (top) maps to image top
            
            # Enhance contrast if requested
            if enhance_contrast:
                enhancer = ImageEnhance.Contrast(img)
                img = enhancer.enhance(CONTRAST_ENHANCEMENT_FACTOR)
            
            # Store converted image
            self.converted_image = img
            
            # Update after label
            self.after_label.config(
                text=f"POV Format: {img.width}x{img.height}px"
            )
            
            # Display converted image (scaled for preview)
            self.display_image_on_canvas(
                self.after_canvas,
                img,
                scale_up=True
            )
            
        except Exception as e:
            messagebox.showerror(
                "Error Converting Image",
                f"Failed to convert image:\n{str(e)}"
            )
            self.update_status("Error converting image")
            
        except Exception as e:
            messagebox.showerror(
                "Error Converting Image",
                f"Failed to convert image:\n{str(e)}"
            )
            self.update_status("Error converting image")
    
    def on_dimension_change(self, changed_field):
        """Handle dimension changes with aspect ratio lock"""
        if not self.aspect_ratio_lock_var.get() or self.original_aspect_ratio == 1.0:
            # Just update preview if not locked or no image loaded
            if self.original_image:
                self.update_preview()
            return
        
        try:
            if changed_field == 'width':
                new_width = self.width_var.get()
                new_height = int(round(new_width * self.original_aspect_ratio))
                new_height = max(1, min(200, new_height))  # Clamp to valid range
                self.height_var.set(new_height)
            elif changed_field == 'height':
                new_height = self.height_var.get()
                new_width = int(round(new_height / self.original_aspect_ratio))
                new_width = max(1, min(100, new_width))  # Clamp to valid range
                self.width_var.set(new_width)
        except (ValueError, ZeroDivisionError):
            pass  # Ignore invalid values during typing
        
        # Update preview
        if self.original_image:
            self.update_preview()
    
    def on_settings_change(self):
        """Handle settings change"""
        if self.original_image:
            self.update_preview()
            self.update_status("Preview updated with new settings")
    
    def convert_and_save(self):
        """Convert current image and save to file"""
        if not self.input_files:
            messagebox.showwarning("No Image", "Please select an image first")
            return
        
        # Get current file
        input_path = self.input_files[self.current_file_index]
        
        # Ask for output location
        default_name = os.path.splitext(os.path.basename(input_path))[0] + "_pov.png"
        output_path = filedialog.asksaveasfilename(
            title="Save converted image",
            defaultextension=".png",
            initialfile=default_name,
            filetypes=(("PNG files", "*.png"), ("All files", "*.*"))
        )
        
        if not output_path:
            return
        
        try:
            self.update_status("Converting and saving...")
            
            # Use the existing conversion function
            # HEIGHT is fixed (display LEDs), WIDTH is calculated
            success = convert_image_for_pov(
                input_path,
                output_path,
                height=self.height_var.get(),
                max_width=self.max_width_var.get(),
                enhance_contrast=self.contrast_var.get(),
                flip_horizontal=self.flip_horizontal_var.get()
            )
            
            if success:
                messagebox.showinfo(
                    "Success",
                    f"Image converted and saved to:\n{output_path}\n\n"
                    "You can now upload this image via the web interface:\n"
                    "1. Connect to POV-POI-WiFi\n"
                    "2. Open http://192.168.4.1\n"
                    "3. Upload your converted image"
                )
                self.update_status(f"Saved: {os.path.basename(output_path)}")
            else:
                messagebox.showerror(
                    "Conversion Failed",
                    "Failed to convert image. Check console for details."
                )
                self.update_status("Conversion failed")
                
        except Exception as e:
            messagebox.showerror(
                "Error",
                f"An error occurred:\n{str(e)}"
            )
            self.update_status("Error during conversion")
    
    def batch_convert_all(self):
        """Convert all selected images"""
        if len(self.input_files) < 2:
            messagebox.showwarning(
                "Not Enough Files",
                "Please select multiple images for batch conversion"
            )
            return
        
        # Ask for output directory
        output_dir = filedialog.askdirectory(
            title="Select output directory for converted images"
        )
        
        if not output_dir:
            return
        
        # Confirm action
        response = messagebox.askyesno(
            "Batch Conversion",
            f"Convert {len(self.input_files)} images to POV format?\n\n"
            f"Output directory: {output_dir}"
        )
        
        if not response:
            return
        
        # Disable buttons during conversion
        self.convert_btn.config(state=tk.DISABLED)
        self.batch_convert_btn.config(state=tk.DISABLED)
        self.select_btn.config(state=tk.DISABLED)
        self.select_multiple_btn.config(state=tk.DISABLED)
        
        # Show progress bar
        self.progress.pack(fill=tk.X, side=tk.BOTTOM)
        self.progress.start(10)
        
        def batch_convert_thread():
            """Thread function for batch conversion"""
            success_count = 0
            failed_files = []
            
            for i, input_path in enumerate(self.input_files):
                filename = os.path.basename(input_path)
                self.update_status(f"Converting {i+1}/{len(self.input_files)}: {filename}")
                
                # Generate output path
                base_name = os.path.splitext(filename)[0]
                output_path = os.path.join(output_dir, f"{base_name}_pov.png")
                
                try:
                    # HEIGHT is fixed (display LEDs), WIDTH is calculated
                    success = convert_image_for_pov(
                        input_path,
                        output_path,
                        height=self.height_var.get(),
                        max_width=self.max_width_var.get(),
                        enhance_contrast=self.contrast_var.get(),
                        flip_horizontal=self.flip_horizontal_var.get()
                    )
                    
                    if success:
                        success_count += 1
                    else:
                        failed_files.append(filename)
                        
                except Exception as e:
                    print(f"Error converting {filename}: {e}")
                    failed_files.append(filename)
            
            # Stop progress bar and re-enable buttons
            self.progress.stop()
            self.progress.pack_forget()
            self.convert_btn.config(state=tk.NORMAL)
            self.batch_convert_btn.config(state=tk.NORMAL)
            self.select_btn.config(state=tk.NORMAL)
            self.select_multiple_btn.config(state=tk.NORMAL)
            
            # Show results
            if failed_files:
                messagebox.showwarning(
                    "Batch Conversion Complete",
                    f"Converted: {success_count}/{len(self.input_files)} images\n\n"
                    f"Failed files:\n" + "\n".join(failed_files[:10]) +
                    ("\n..." if len(failed_files) > 10 else "")
                )
            else:
                messagebox.showinfo(
                    "Batch Conversion Complete",
                    f"Successfully converted all {success_count} images!\n\n"
                    f"Output directory: {output_dir}"
                )
            
            self.update_status(f"Batch conversion complete: {success_count}/{len(self.input_files)} successful")
        
        # Run conversion in separate thread to keep UI responsive
        thread = Thread(target=batch_convert_thread)
        thread.daemon = True
        thread.start()


def main():
    """Main function"""
    root = tk.Tk()
    app = POVImageConverterGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
