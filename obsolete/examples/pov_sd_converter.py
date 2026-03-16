#!/usr/bin/env python3
"""
POV SD Card Converter - Cross-Platform GUI

Converts images to .pov format for direct upload to the microSD card
used by the Nebula Poi Teensy. No web interface needed - copy .pov files
to the SD card /images/ folder, then load via the web UI.

Runs on Windows, macOS, and Linux. Requires Python 3.6+ and Pillow.

Usage:
  python pov_sd_converter.py

Or double-click the script (if Python is in PATH).
"""

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import os
import sys
from threading import Thread

try:
    from PIL import Image, ImageTk, ImageEnhance
except ImportError:
    root = tk.Tk()
    root.withdraw()
    messagebox.showerror(
        "Missing Dependency",
        "Pillow is required.\n\n"
        "Install with:\n  pip install Pillow\n\n"
        "Or: python -m pip install Pillow"
    )
    sys.exit(1)

try:
    from image_converter import save_as_pov, convert_image_to_pov_data, SD_MAX_DIMENSION
except ImportError:
    from importlib.util import spec_from_file_location, module_from_spec
    _spec = spec_from_file_location("image_converter",
        os.path.join(os.path.dirname(__file__), "image_converter.py"))
    _mod = module_from_spec(_spec)
    _spec.loader.exec_module(_mod)
    save_as_pov = _mod.save_as_pov
    convert_image_to_pov_data = _mod.convert_image_to_pov_data
    SD_MAX_DIMENSION = getattr(_mod, "SD_MAX_DIMENSION", 1024)

SD_INSTRUCTIONS = (
    "1. Copy the .pov files to your SD card\n"
    "2. Create folder: /images/ (if it doesn't exist)\n"
    "3. Place .pov files in /images/\n"
    "4. Insert SD card into Teensy, connect to web UI\n"
    "5. Use 'Refresh List' and 'Load' in SD Card Storage section\n\n"
    "PlatformIO firmware uses /images/ (Arduino IDE may use /poi_images/)"
)


class POVSDConverterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("POV SD Card Converter")
        self.root.geometry("800x680")
        self.root.resizable(True, True)

        self.input_files = []
        self.current_index = 0
        self.original_image = None
        self.converted_preview = None
        self.original_aspect_ratio = 1.0

        # Settings
        self.height_var = tk.IntVar(value=32)
        self.max_width_var = tk.IntVar(value=400)
        self.contrast_var = tk.BooleanVar(value=True)
        self.flip_h_var = tk.BooleanVar(value=False)
        self.flip_v_var = tk.BooleanVar(value=False)

        self.setup_ui()

    def setup_ui(self):
        # Header
        header = tk.Frame(self.root, bg="#1a5f2a", height=50)
        header.pack(fill=tk.X)
        header.pack_propagate(False)
        tk.Label(
            header, text="POV SD Card Converter",
            font=("Arial", 16, "bold"), bg="#1a5f2a", fg="white"
        ).pack(pady=10)
        tk.Label(
            header, text="Convert images to .pov for direct SD card upload",
            font=("Arial", 9), bg="#1a5f2a", fg="#b8e0c8"
        ).pack()

        main = tk.Frame(self.root, padx=20, pady=15)
        main.pack(fill=tk.BOTH, expand=True)

        # File selection
        btn_frame = tk.Frame(main)
        btn_frame.pack(fill=tk.X, pady=(0, 10))
        tk.Button(
            btn_frame, text="Select Image(s)", command=self.select_files,
            bg="#2e7d32", fg="white", font=("Arial", 10), padx=15, pady=8, cursor="hand2"
        ).pack(side=tk.LEFT, padx=(0, 8))
        tk.Button(
            btn_frame, text="Select Output Folder", command=self.select_output_dir,
            bg="#455a64", fg="white", font=("Arial", 10), padx=15, pady=8, cursor="hand2"
        ).pack(side=tk.LEFT)
        self.output_dir_var = tk.StringVar(value=os.path.expanduser("~"))
        self.output_label = tk.Label(main, text="", fg="#666", font=("Arial", 9))
        self.output_label.pack(anchor="w")

        # Preview area
        preview_frame = tk.LabelFrame(main, text="Preview", padx=10, pady=10)
        preview_frame.pack(fill=tk.BOTH, expand=True, pady=10)
        self.preview_canvas = tk.Canvas(preview_frame, width=400, height=200, bg="#222")
        self.preview_canvas.pack(fill=tk.BOTH, expand=True)
        self.file_label = tk.Label(preview_frame, text="No image selected", fg="#888")
        self.file_label.pack(pady=5)
        self.dim_label = tk.Label(preview_frame, text="", fg="#666", font=("Arial", 9))
        self.dim_label.pack()

        # Settings
        settings = tk.LabelFrame(main, text="Settings", padx=15, pady=10)
        settings.pack(fill=tk.X, pady=(0, 10))

        row1 = tk.Frame(settings)
        row1.pack(fill=tk.X)
        tk.Label(row1, text="Height (LEDs):", width=12, anchor="w").pack(side=tk.LEFT)
        tk.Spinbox(row1, from_=1, to=SD_MAX_DIMENSION, textvariable=self.height_var,
                   width=8, command=self.on_change).pack(side=tk.LEFT, padx=5)
        tk.Label(row1, text="Max width:", width=10, anchor="w").pack(side=tk.LEFT, padx=(20, 0))
        tk.Spinbox(row1, from_=1, to=SD_MAX_DIMENSION, textvariable=self.max_width_var,
                   width=8, command=self.on_change).pack(side=tk.LEFT, padx=5)
        tk.Label(row1, text="(max 1024 for SD)", fg="#888", font=("Arial", 8)).pack(side=tk.LEFT)

        row2 = tk.Frame(settings)
        row2.pack(fill=tk.X, pady=(8, 0))
        tk.Checkbutton(row2, text="Enhance contrast", variable=self.contrast_var,
                       command=self.on_change).pack(side=tk.LEFT)
        tk.Checkbutton(row2, text="Flip horizontal", variable=self.flip_h_var,
                       command=self.on_change).pack(side=tk.LEFT, padx=(15, 0))
        tk.Checkbutton(row2, text="Flip vertical", variable=self.flip_v_var,
                       command=self.on_change).pack(side=tk.LEFT, padx=(15, 0))

        # Actions
        action_frame = tk.Frame(main)
        action_frame.pack(fill=tk.X, pady=10)
        self.convert_btn = tk.Button(
            action_frame, text="Convert & Save .pov", command=self.convert_save,
            bg="#1b5e20", fg="white", font=("Arial", 11, "bold"),
            padx=25, pady=10, cursor="hand2", state=tk.DISABLED
        )
        self.convert_btn.pack(side=tk.LEFT, padx=(0, 10))
        self.batch_btn = tk.Button(
            action_frame, text="Batch Convert All", command=self.batch_convert,
            bg="#f57c00", fg="white", font=("Arial", 11), padx=20, pady=10,
            cursor="hand2", state=tk.DISABLED
        )
        self.batch_btn.pack(side=tk.LEFT)
        tk.Button(
            action_frame, text="SD Card Instructions", command=self.show_instructions,
            font=("Arial", 10), padx=15, pady=10
        ).pack(side=tk.LEFT, padx=(20, 0))

        self.progress = ttk.Progressbar(main, mode="indeterminate")
        self.status = tk.Label(main, text="Ready. Select image(s) and output folder.", fg="#555")
        self.status.pack(anchor="w", pady=(5, 0))

    def select_output_dir(self):
        d = filedialog.askdirectory(title="Select output folder for .pov files")
        if d:
            self.output_dir_var.set(d)
            self.output_label.config(text=f"Output: {d}")

    def select_files(self):
        paths = filedialog.askopenfilenames(
            title="Select images",
            filetypes=(("Images", "*.jpg *.jpeg *.png *.gif *.bmp"), ("All", "*.*"))
        )
        if paths:
            self.input_files = list(paths)
            self.current_index = 0
            self.load_preview()
            self.convert_btn.config(state=tk.NORMAL)
            self.batch_btn.config(state=tk.NORMAL if len(self.input_files) > 1 else tk.DISABLED)
            self.status.config(text=f"Loaded {len(self.input_files)} file(s)")

    def load_preview(self):
        if not self.input_files:
            return
        path = self.input_files[self.current_index]
        try:
            self.original_image = Image.open(path)
            if self.original_image.mode != "RGB":
                self.original_image = self.original_image.convert("RGB")
            self.original_aspect_ratio = self.original_image.height / self.original_image.width
            self.update_preview()
            self.file_label.config(text=os.path.basename(path))
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def on_change(self):
        if self.original_image:
            self.update_preview()

    def update_preview(self):
        if not self.original_image:
            return
        try:
            h = max(1, min(SD_MAX_DIMENSION, self.height_var.get()))
            mw = max(1, min(SD_MAX_DIMENSION, self.max_width_var.get()))
            result = convert_image_to_pov_data(
                self.input_files[self.current_index],
                height=h, max_width=mw,
                enhance_contrast=self.contrast_var.get(),
                flip_horizontal=self.flip_h_var.get(),
                flip_vertical=self.flip_v_var.get()
            )
            if result:
                w, h, _ = result
                self.dim_label.config(text=f"Output: {w}x{h} px")
                # Build tiny preview from RGB data
                from PIL import Image as PILImage
                img = PILImage.frombytes("RGB", (w, h), result[2])
                scale = min(350 // max(w, 1), 350 // max(h, 1), 10)
                if scale < 1:
                    scale = 1
                disp = img.resize((w * scale, h * scale), PILImage.NEAREST)
                self.converted_preview = ImageTk.PhotoImage(disp)
                self.preview_canvas.delete("all")
                cw = self.preview_canvas.winfo_width()
                ch = self.preview_canvas.winfo_height()
                x = (cw - disp.width) // 2
                y = (ch - disp.height) // 2
                self.preview_canvas.create_image(max(0, x), max(0, y),
                    image=self.converted_preview, anchor=tk.NW)
        except Exception:
            pass

    def convert_save(self):
        if not self.input_files:
            return
        path = self.input_files[self.current_index]
        out_dir = self.output_dir_var.get()
        base = os.path.splitext(os.path.basename(path))[0]
        out_path = os.path.join(out_dir, f"{base}.pov")
        try:
            success = save_as_pov(
                path, out_path,
                height=max(1, min(SD_MAX_DIMENSION, self.height_var.get())),
                max_width=max(1, min(SD_MAX_DIMENSION, self.max_width_var.get())),
                enhance_contrast=self.contrast_var.get(),
                flip_horizontal=self.flip_h_var.get(),
                flip_vertical=self.flip_v_var.get()
            )
            if success:
                self.status.config(text=f"Saved: {out_path}")
                messagebox.showinfo(
                    "Saved",
                    f"Saved to:\n{out_path}\n\nCopy to SD card /images/ folder."
                )
            else:
                messagebox.showerror("Error", "Conversion failed.")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def batch_convert(self):
        if len(self.input_files) < 2:
            messagebox.showinfo("Info", "Select multiple images for batch conversion.")
            return
        out_dir = self.output_dir_var.get()
        if not os.path.isdir(out_dir):
            messagebox.showerror("Error", "Select a valid output folder first.")
            return
        self.convert_btn.config(state=tk.DISABLED)
        self.batch_btn.config(state=tk.DISABLED)
        self.progress.pack(fill=tk.X, pady=5)
        self.progress.start(8)

        def run():
            ok = 0
            for p in self.input_files:
                base = os.path.splitext(os.path.basename(p))[0]
                out = os.path.join(out_dir, f"{base}.pov")
                try:
                    if save_as_pov(
                        p, out,
                        height=max(1, min(SD_MAX_DIMENSION, self.height_var.get())),
                        max_width=max(1, min(SD_MAX_DIMENSION, self.max_width_var.get())),
                        enhance_contrast=self.contrast_var.get(),
                        flip_horizontal=self.flip_h_var.get(),
                        flip_vertical=self.flip_v_var.get()
                    ):
                        ok += 1
                except Exception:
                    pass
            self.progress.stop()
            self.progress.pack_forget()
            self.convert_btn.config(state=tk.NORMAL)
            self.batch_btn.config(state=tk.NORMAL)
            self.status.config(text=f"Batch complete: {ok}/{len(self.input_files)} converted")
            messagebox.showinfo(
                "Batch Complete",
                f"Converted {ok} of {len(self.input_files)} images.\n\n"
                f"Output: {out_dir}\n\n{SD_INSTRUCTIONS}"
            )

        Thread(target=run, daemon=True).start()

    def show_instructions(self):
        messagebox.showinfo("SD Card Instructions", SD_INSTRUCTIONS)


def main():
    root = tk.Tk()
    app = POVSDConverterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
