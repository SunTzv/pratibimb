#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <memory>
#include <stdexcept>
#include <array>
#endif
#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

// --- AGGRESSIVE LOGGER ---
void logDebug(const string& msg) {
#ifdef _WIN32
    const char* tempEnv = getenv("TEMP");
    string tempDir = tempEnv ? tempEnv : "C:\\Temp";
    string logPath = tempDir + "\\pratibimb_log.txt";
#else
    string logPath = "/tmp/pratibimb_log.txt";
#endif
    ofstream logFile(logPath, ios::app);
    logFile << msg << "\n";
    logFile.close();
}

void sendMessage(const string& jsonResponse) {
    uint32_t len = static_cast<uint32_t>(jsonResponse.length());
    cout.write(reinterpret_cast<const char*>(&len), 4);
    cout.write(jsonResponse.c_str(), len);
    cout.flush();
    logDebug("[SUCCESS] Sent chunk to browser. Length: " + to_string(len));
}

#ifndef _WIN32
string exec(const char* cmd) {
    std::array<char, 128> buffer;
    string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') result.pop_back();
    if (result.rfind("file://", 0) == 0) result = result.substr(7);
    if (!result.empty() && result.front() == '\'' && result.back() == '\'') {
        result = result.substr(1, result.length() - 2);
        if (result.rfind("file://", 0) == 0) result = result.substr(7);
    }
    return result;
}

string getLinuxWallpaperPath() {
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    string desktopStr = desktop ? desktop : "";
    
    if (desktopStr.find("GNOME") != string::npos || desktopStr.find("Unity") != string::npos || desktopStr.find("Pantheon") != string::npos || desktopStr.find("Budgie") != string::npos) {
        string path = exec("gsettings get org.gnome.desktop.background picture-uri-dark 2>/dev/null | tr -d \"'\"");
        if (path.empty() || path == "") {
            path = exec("gsettings get org.gnome.desktop.background picture-uri 2>/dev/null | tr -d \"'\"");
        }
        if (path.rfind("file://", 0) == 0) path = path.substr(7);
        return path;
    } else if (desktopStr.find("KDE") != string::npos) {
        string path = exec("kreadconfig5 --file plasma-org.kde.plasma.desktop-appletsrc --group Containments --group 1 --group Wallpaper --group org.kde.image --group General --key Image 2>/dev/null");
        if (path.empty()) {
            path = exec("grep '^Image=' ~/.config/plasma-org.kde.plasma.desktop-appletsrc 2>/dev/null | tail -n 1 | cut -d '=' -f 2 | tr -d '\\n'");
        }
        if (path.rfind("file://", 0) == 0) path = path.substr(7);
        return path;
    } else if (desktopStr.find("XFCE") != string::npos) {
        return exec("xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/workspace0/last-image 2>/dev/null");
    } else if (desktopStr.find("MATE") != string::npos) {
        return exec("gsettings get org.mate.background picture-filename 2>/dev/null | tr -d \"'\"");
    } else if (desktopStr.find("Cinnamon") != string::npos || desktopStr.find("X-Cinnamon") != string::npos) {
        string path = exec("gsettings get org.cinnamon.desktop.background picture-uri 2>/dev/null | tr -d \"'\"");
        if (path.rfind("file://", 0) == 0) path = path.substr(7);
        return path;
    } else if (desktopStr.find("LXDE") != string::npos) {
        return exec("grep '^wallpaper=' ~/.config/pcmanfm/LXDE/desktop.conf 2>/dev/null | cut -d '=' -f 2");
    } else if (desktopStr.find("LXQt") != string::npos) {
        return exec("grep '^icon=' ~/.config/pcmanfm-qt/lxqt/settings.conf 2>/dev/null | cut -d '=' -f 2");
    } else if (desktopStr.find("Deepin") != string::npos) {
        string path = exec("dconf read /com/deepin/wrap/gnome/desktop/background/picture-uri 2>/dev/null | tr -d \"'\"");
        if (path.rfind("file://", 0) == 0) path = path.substr(7);
        return path;
    }
    
    string fehPath = exec("grep '^feh --bg' ~/.fehbg 2>/dev/null | awk '{print $NF}' | tr -d \"'\"");
    if (!fehPath.empty()) return fehPath;

    return "";
}
#endif

static const string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

string base64_encode(const vector<unsigned char>& buf) {
    string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    for (size_t n = 0; n < buf.size(); n++) {
        char_array_3[i++] = buf[n];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while ((i++ < 3)) ret += '=';
    }
    return ret;
}

void stbi_write_mem(void *context, void *data, int size) {
    vector<unsigned char> *buf = static_cast<vector<unsigned char> *>(context);
    unsigned char *ptr = static_cast<unsigned char *>(data);
    buf->insert(buf->end(), ptr, ptr + size);
}

int main() {
    logDebug("=====================================");
    logDebug("[STEP 1] Host Executable Launched by Browser!");

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    uint32_t length = 0;
    
    logDebug("[STEP 2] Waiting for JavaScript to send a message...");
    
    // THE HEARTBEAT LOOP
    while (cin.read(reinterpret_cast<char*>(&length), 4)) {
        if (length == 0) {
            logDebug("[WARNING] Received empty length block. Skipping.");
            continue;
        }

        logDebug("[STEP 3] Received message length from JS: " + to_string(length));

        vector<char> msg(length);
        cin.read(msg.data(), length);
        logDebug("[STEP 4] Read payload from JS.");

        string wallpaperPath;
#ifdef _WIN32
        const char* appDataEnv = getenv("APPDATA");
        string appData = appDataEnv ? appDataEnv : "";
        wallpaperPath = appData + "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";
#else
        wallpaperPath = getLinuxWallpaperPath();
#endif

        ifstream file(wallpaperPath, ios::binary);
        if (!file) {
            logDebug("[ERROR] Could not find Windows Wallpaper at path.");
            sendMessage("{\"chunk\": \"\", \"done\": true, \"error\": \"not_found\"}");
            continue;
        }

        vector<unsigned char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close(); 
        logDebug("[STEP 5] Wallpaper loaded into memory. Size: " + to_string(buffer.size()));
        
        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(buffer.data(), buffer.size(), &width, &height, &channels, 3);
        
        string base64Image;
        if (pixels) {
            logDebug("[STEP 6] Image parsed by STB. Encoding to JPG...");
            vector<unsigned char> compressed_buffer;
            stbi_write_jpg_to_func(stbi_write_mem, &compressed_buffer, width, height, 3, pixels, 80);
            stbi_image_free(pixels);
            base64Image = base64_encode(compressed_buffer);
        } else {
            logDebug("[WARNING] STB failed to parse image. Falling back to raw base64.");
            base64Image = base64_encode(buffer);
        }

        logDebug("[STEP 7] Base64 encoding complete. Total string length: " + to_string(base64Image.length()));

        size_t chunkSize = 500000; 
        for (size_t i = 0; i < base64Image.length(); i += chunkSize) {
            string chunk = base64Image.substr(i, chunkSize);
            bool isDone = (i + chunkSize >= base64Image.length());
            
            string jsonResponse = "{\"chunk\": \"" + chunk + "\", \"done\": " + (isDone ? "true" : "false") + "}";
            sendMessage(jsonResponse);
        }
        logDebug("[STEP 8] Finished sending all chunks.");
    }

    logDebug("[FATAL] cin.read() failed. Browser closed the connection or pipe broke.");
    return 0;
}