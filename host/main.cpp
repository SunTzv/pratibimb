#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

// --- AGGRESSIVE LOGGER ---
void logDebug(const string& msg) {
    string logPath = string(getenv("TEMP")) + "\\pratibimb_log.txt";
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

    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

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

        string appData = getenv("APPDATA");
        string wallpaperPath = appData + "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";

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