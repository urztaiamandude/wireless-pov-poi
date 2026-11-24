import { fitPovDimensions, POV_IMAGE_MAX_HEIGHT } from './displayLimits';

/** Raw RGB (top to bottom) for POST /api/image. The filename carries the dimensions. */
export function createRawRGB(canvas: HTMLCanvasElement): { blob: Blob; filename: string } {
  const ctx = canvas.getContext('2d', { willReadFrequently: true });
  if (!ctx) throw new Error('Canvas unavailable');
  const width = canvas.width;
  const height = canvas.height;
  const imgData = ctx.getImageData(0, 0, width, height).data;
  const buffer = new Uint8Array(width * height * 3);
  let offset = 0;
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      buffer[offset++] = imgData[i];
      buffer[offset++] = imgData[i + 1];
      buffer[offset++] = imgData[i + 2];
    }
  }
  return {
    blob: new Blob([buffer], { type: 'application/octet-stream' }),
    filename: `image_${width}x${height}.rgb`,
  };
}

function loadHtmlImage(url: string): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = () => reject(new Error('Image decode failed'));
    img.src = url;
  });
}

/** Decode an image file and export it as a height-clamped raw RGB upload. */
export async function imageFileToRawRgb(
  file: Blob,
  ledHeight: number = POV_IMAGE_MAX_HEIGHT,
): Promise<{ blob: Blob; filename: string }> {
  const url = URL.createObjectURL(file);
  try {
    const img = await loadHtmlImage(url);
    const { width, height } = fitPovDimensions(img.naturalWidth, img.naturalHeight, ledHeight);
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    if (!ctx) throw new Error('Canvas unavailable');
    ctx.drawImage(img, 0, 0, width, height);
    return createRawRGB(canvas);
  } finally {
    URL.revokeObjectURL(url);
  }
}
