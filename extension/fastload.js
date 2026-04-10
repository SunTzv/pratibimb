// 1. Instantly apply the dominant color (0ms execution)
const dominantColor = localStorage.getItem('wallpaperColor');
if (dominantColor) {
    document.documentElement.style.backgroundColor = dominantColor;
}

// 2. Start decoding and painting the heavy image (~300ms execution)
const fastCache = localStorage.getItem('instantWallpaper');
if (fastCache) {
    document.documentElement.style.backgroundImage = `url('${fastCache}')`;
}