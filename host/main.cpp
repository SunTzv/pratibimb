#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <memory>
#include <stdexcept>
#include <array>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#endif
#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

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

string getExecutableDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string path(buffer);
    return path.substr(0, path.find_last_of("\\/"));
#else
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    string path(buffer, (count > 0) ? count : 0);
    return path.substr(0, path.find_last_of("\\/"));
#endif
}

string getWallpapersDir() {
    string exeDir = getExecutableDir();
#ifdef _WIN32
    return exeDir + "\\..\\wallpapers";
#else
    return exeDir + "/../wallpapers";
#endif
}

#ifndef _WIN32
string exec(const char* cmd) {
    std::array<char, 128> buffer;
    string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
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
    string currentFile = getExecutableDir() + "/../current_wallpaper.txt";
    ifstream cf(currentFile);
    if (cf) {
        string path;
        getline(cf, path);
        if (!path.empty()) return path;
    }
    
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
    }
    return "";
}

void setLinuxWallpaper(const string& path) {
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    string desktopStr = desktop ? desktop : "";
    
    string currentFile = getExecutableDir() + "/../current_wallpaper.txt";
    ofstream cf(currentFile);
    if (cf) cf << path;

    if (desktopStr.find("GNOME") != string::npos || desktopStr.find("Unity") != string::npos || desktopStr.find("Pantheon") != string::npos || desktopStr.find("Budgie") != string::npos) {
        exec(("gsettings set org.gnome.desktop.background picture-uri-dark \"file://" + path + "\"").c_str());
        exec(("gsettings set org.gnome.desktop.background picture-uri \"file://" + path + "\"").c_str());
    } else if (desktopStr.find("KDE") != string::npos) {
        exec(("qdbus org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript \"var Desktops = desktops(); for (i=0;i<Desktops.length;i++) { d = Desktops[i]; d.wallpaperPlugin = 'org.kde.image'; d.currentConfigGroup = Array('Wallpaper', 'org.kde.image', 'General'); d.writeConfig('Image', 'file://" + path + "'); }\"").c_str());
    } else if (desktopStr.find("Cinnamon") != string::npos || desktopStr.find("X-Cinnamon") != string::npos) {
        exec(("gsettings set org.cinnamon.desktop.background picture-uri \"file://" + path + "\"").c_str());
    }
}
#else
string getWindowsWallpaperPath() {
    string currentFile = getExecutableDir() + "\\..\\current_wallpaper.txt";
    ifstream cf(currentFile);
    if (cf) {
        string path;
        getline(cf, path);
        if (!path.empty()) return path;
    }

    char path[MAX_PATH];
    DWORD pathSize = sizeof(path);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "Wallpaper", NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return string(path);
        }
        RegCloseKey(hKey);
    }
    
    const char* appDataEnv = getenv("APPDATA");
    string appData = appDataEnv ? appDataEnv : "";
    return appData + "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";
}

void setWindowsWallpaper(const string& path) {
    string currentFile = getExecutableDir() + "\\..\\current_wallpaper.txt";
    ofstream cf(currentFile);
    if (cf) cf << path;

    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)path.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
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

string extractJsonValue(const string& json, const string& key) {
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == string::npos) {
        keyPos = json.find(key + "\""); // check without starting quote sometimes JS sends {action: "..."}
        if (keyPos == string::npos) return "";
    }
    size_t colonPos = json.find(":", keyPos);
    if (colonPos == string::npos) return "";
    size_t quoteStart = json.find("\"", colonPos);
    if (quoteStart == string::npos) return "";
    size_t quoteEnd = json.find("\"", quoteStart + 1);
    if (quoteEnd == string::npos) return "";
    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

void processGetWallpaper(const string& wallpaperPath, const string& requestedFilename = "") {
    ifstream file(wallpaperPath, ios::binary);
    if (!file) {
        logDebug("[ERROR] Could not find Wallpaper at path: " + wallpaperPath);
        sendMessage("{\"chunk\": \"\", \"done\": true, \"error\": \"not_found\"}");
        return;
    }

    vector<unsigned char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close(); 
    logDebug("[STEP 5] Wallpaper loaded into memory. Size: " + to_string(buffer.size()));
    
    bool isGif = false;
    string mimeType = "image/jpeg";
    if (wallpaperPath.length() >= 4) {
        string ext = wallpaperPath.substr(wallpaperPath.length() - 4);
        for(auto& c : ext) c = tolower(c);
        if (ext == ".gif") {
            isGif = true;
            mimeType = "image/gif";
        }
    }
    if (buffer.size() >= 6 && buffer[0] == 'G' && buffer[1] == 'I' && buffer[2] == 'F') {
        isGif = true;
        mimeType = "image/gif";
    }

    string base64Image;
    if (isGif) {
        logDebug("[STEP 6] GIF detected. Skipping JPEG compression.");
        base64Image = base64_encode(buffer);
    } else {
        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(buffer.data(), buffer.size(), &width, &height, &channels, 3);
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
    }

    logDebug("[STEP 7] Base64 encoding complete. Total string length: " + to_string(base64Image.length()));

    size_t chunkSize = 500000; 
    for (size_t i = 0; i < base64Image.length(); i += chunkSize) {
        string chunk = base64Image.substr(i, chunkSize);
        bool isDone = (i + chunkSize >= base64Image.length());
        
        string jsonResponse = "{\"chunk\": \"" + chunk + "\", \"done\": " + (isDone ? "true" : "false") + ", \"mime\": \"" + mimeType + "\"";
        if (!requestedFilename.empty()) {
            jsonResponse += ", \"filename\": \"" + requestedFilename + "\"";
        }
        jsonResponse += "}";
        sendMessage(jsonResponse);
    }
}

void processListWallpapers() {
    string wDir = getWallpapersDir();
    vector<string> files;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((wDir + "\\*").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            string name = findData.cFileName;
            if (name != "." && name != "..") files.push_back(name);
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(wDir.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name != "." && name != "..") files.push_back(name);
        }
        closedir(dir);
    }
#endif

    string jsonResponse = "{\"action\": \"list_wallpapers\", \"files\": [";
    for (size_t i = 0; i < files.size(); i++) {
        jsonResponse += "\"" + files[i] + "\"";
        if (i < files.size() - 1) jsonResponse += ", ";
    }
    jsonResponse += "]}";
    sendMessage(jsonResponse);
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
    
    while (cin.read(reinterpret_cast<char*>(&length), 4)) {
        if (length == 0) continue;

        vector<char> msgBuf(length + 1);
        cin.read(msgBuf.data(), length);
        msgBuf[length] = '\0';
        string msg(msgBuf.data());
        logDebug("[STEP 4] Read payload from JS: " + msg);

        string action = extractJsonValue(msg, "action");
        if (action.empty()) action = extractJsonValue(msg, "text"); // fallback

        if (action == "get_wallpaper") {
#ifdef _WIN32
            string wallpaperPath = getWindowsWallpaperPath();
#else
            string wallpaperPath = getLinuxWallpaperPath();
#endif
            processGetWallpaper(wallpaperPath);
        } else if (action == "list_wallpapers") {
            processListWallpapers();
        } else if (action == "set_wallpaper") {
            string filename = extractJsonValue(msg, "filename");
            string wDir = getWallpapersDir();
#ifdef _WIN32
            string fullPath = wDir + "\\" + filename;
            setWindowsWallpaper(fullPath);
#else
            string fullPath = wDir + "/" + filename;
            setLinuxWallpaper(fullPath);
#endif
            processGetWallpaper(fullPath);
        } else if (action == "get_preview") {
            string filename = extractJsonValue(msg, "filename");
            string wDir = getWallpapersDir();
#ifdef _WIN32
            string fullPath = wDir + "\\" + filename;
#else
            string fullPath = wDir + "/" + filename;
#endif
            processGetWallpaper(fullPath, filename);
        } else if (action == "open_folder") {
            string wDir = getWallpapersDir();
#ifdef _WIN32
            string cmd = "explorer \"" + wDir + "\"";
            system(cmd.c_str());
#else
            string cmd = "xdg-open \"" + wDir + "\" > /dev/null 2>&1 &";
            system(cmd.c_str());
#endif
            sendMessage("{\"action\": \"open_folder\", \"status\": \"success\"}");
        }
    }

    logDebug("[FATAL] cin.read() failed. Browser closed the connection.");
    return 0;
}
