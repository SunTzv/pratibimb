#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>

using namespace std;

// Fast, self-contained Base64 Encoder
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

int main() {
    // CRITICAL: Force binary mode so Windows doesn't corrupt the Native Messaging output
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    // 1. Read the 32-bit message length from Brave
    unsigned int length = 0;
    cin.read(reinterpret_cast<char*>(&length), 4);
    if (length == 0) return 0;

    // 2. Read the actual message payload (we discard it, but we must read it to clear the buffer)
    vector<char> msg(length);
    cin.read(msg.data(), length);

    // 3. Locate the current Windows Wallpaper
    string appData = getenv("APPDATA");
    string wallpaperPath = appData + "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";

    // 4. Read the image file
    ifstream file(wallpaperPath, ios::binary);
    if (!file) {
        // If it fails, send an empty image so the browser doesn't crash
        string err = "{\"image\": \"\"}";
        unsigned int len = err.length();
        cout.write(reinterpret_cast<char*>(&len), 4);
        cout << err;
        return 1;
    }

    // Load file into a buffer
    vector<unsigned char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    // 5. Encode to Base64 and format as JSON
    string base64Image = base64_encode(buffer);
    string jsonResponse = "{\"image\": \"data:image/jpeg;base64," + base64Image + "\"}";

    // 6. Send the payload length, followed by the JSON payload back to Brave
    unsigned int responseLength = jsonResponse.length();
    cout.write(reinterpret_cast<char*>(&responseLength), 4);
    cout << jsonResponse;

    return 0;
}