#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include <cstdint> // Required for uint32_t

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

// --- THE SAFE WRITER ---
// Always use raw binary writes for BOTH the header and the payload.
void sendMessage(const string& jsonResponse) {
    uint32_t len = static_cast<uint32_t>(jsonResponse.length());
    cout.write(reinterpret_cast<const char*>(&len), 4);
    cout.write(jsonResponse.c_str(), len);
    cout.flush();
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
    // Force Binary Mode to prevent Windows from turning \n into \r\n
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    uint32_t length = 0;
    
    // --- THE HEARTBEAT LOOP ---
    // This keeps the host alive as long as the browser port is open.
    // It only exits when the browser drops the connection (cin fails).
    while (cin.read(reinterpret_cast<char*>(&length), 4)) {
        if (length == 0) continue;

        // Clear the incoming message from the buffer
        vector<char> msg(length);
        cin.read(msg.data(), length);

        string appData = getenv("APPDATA");
        string wallpaperPath = appData + "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";

        ifstream file(wallpaperPath, ios::binary);
        if (!file) {
            sendMessage("{\"chunk\": \"\", \"done\": true, \"error\": \"not_found\"}");
            continue;
        }

        vector<unsigned char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close(); // Close immediately to free memory
        
        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(buffer.data(), buffer.size(), &width, &height, &channels, 3);
        
        string base64Image;
        if (pixels) {
            vector<unsigned char> compressed_buffer;
            stbi_write_jpg_to_func(stbi_write_mem, &compressed_buffer, width, height, 3, pixels, 80);
            stbi_image_free(pixels);
            base64Image = base64_encode(compressed_buffer);
        } else {
            base64Image = base64_encode(buffer);
        }

        size_t chunkSize = 500000; 
        for (size_t i = 0; i < base64Image.length(); i += chunkSize) {
            string chunk = base64Image.substr(i, chunkSize);
            bool isDone = (i + chunkSize >= base64Image.length());
            
            string jsonResponse = "{\"chunk\": \"" + chunk + "\", \"done\": " + (isDone ? "true" : "false") + "}";
            sendMessage(jsonResponse);
        }
    }

    return 0;
}