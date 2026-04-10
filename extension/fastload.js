const fastCache = localStorage.getItem('instantWallpaper');
if (fastCache) {
    document.documentElement.style.backgroundImage = `url('${fastCache}')`;
}