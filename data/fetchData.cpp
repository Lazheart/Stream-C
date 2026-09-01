#include <iostream>
#include <fstream>
#include <curl/curl.h>

using namespace std;

const string fileId = "1Gae6uXFvvu5FbVNw3H2l69OpByFALqBd";
const string outputFile = "data/data.csv";

size_t writeData(void* ptr, size_t size, size_t nmemb, void* stream) {
    ofstream* file = static_cast<ofstream*>(stream);
    file->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

void downloadFile() {
    cout << "Downloading file..." << endl;

    string url =
        "https://drive.usercontent.google.com/download?id=" +
        fileId +
        "&export=download&confirm=t";

    ofstream file(outputFile, ios::binary);

    if (!file) {
        cerr << "Could not create " << outputFile << endl;
        exit(1);
    }

    CURL* curl = curl_easy_init();

    if (!curl) {
        cerr << "Could not initialize CURL." << endl;
        exit(1);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        cerr << "Download failed: "
             << curl_easy_strerror(result) << endl;

        curl_easy_cleanup(curl);
        file.close();
        exit(1);
    }

    curl_easy_cleanup(curl);
    file.close();
}

int main() {

    downloadFile();

    cout << "Done! (saved as: " << outputFile << ")" << endl;

    return 0;
}