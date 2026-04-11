// 1. Instantly inject the dynamic UI colors AND the background color (0ms)
const palette = localStorage.getItem('dynamicPalette');
if (palette) {
    document.documentElement.style.cssText += palette;
}

// 2. Instantly start loading the heavy high-res image
const fastCache = localStorage.getItem('instantWallpaper');
if (fastCache) {
    document.documentElement.style.backgroundImage = `url('${fastCache}')`;
}