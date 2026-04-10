#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>

using namespace std;

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
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    unsigned int length = 0;
    cin.read(reinterpret_cast<char*>(&length), 4);
    if (length == 0) return 0;

    vector<char> msg(length);
    cin.read(msg.data(), length);

    string appData = getenv("APPDATA");
    string wallpaperPath = appData + "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";

    ifstream file(wallpaperPath, ios::binary);
    if (!file) {
        string err = "{\"chunk\": \"\", \"done\": true}";
        unsigned int len = err.length();
        cout.write(reinterpret_cast<char*>(&len), 4);
        cout << err;
        cout.flush();
        return 1;
    }

    vector<unsigned char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    string base64Image = base64_encode(buffer);

    size_t chunkSize = 500000; 
    for (size_t i = 0; i < base64Image.length(); i += chunkSize) {
        string chunk = base64Image.substr(i, chunkSize);
        bool isDone = (i + chunkSize >= base64Image.length());
        
        string jsonResponse = "{\"chunk\": \"" + chunk + "\", \"done\": " + (isDone ? "true" : "false") + "}";
        
        unsigned int responseLength = jsonResponse.length();
        cout.write(reinterpret_cast<char*>(&responseLength), 4);
        cout << jsonResponse;
        cout.flush(); 
    }

    return 0;
}