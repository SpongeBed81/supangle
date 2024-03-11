#include "v8.h"
#include <string>
#include <filesystem>
#include <vector>

using namespace std;

class Fs
{
public:

// Function to read and return the content of a file as a string
    static string readFile(const char* filename) {
    FILE *file = fopen(filename, "r");  // Open file in read mode
    if (file == nullptr) {
        perror("Error opening file");
        return "";
    }

    // Read the file using fseek and rewind
    fseek(file, 0, SEEK_END);  // Move the file pointer to the end of the file
    long fileSize = ftell(file);  // Get the file size
    rewind(file);  // Rewind the file pointer to the beginning

    char *buffer = new char[fileSize + 1];  // Allocate memory to store the content

    // Read the content of the file into the buffer
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize) {
        perror("Error reading file");
        fclose(file);
        delete[] buffer;
        return "";
    }

    buffer[fileSize] = '\0';  // Null-terminate the buffer to make it a valid C-string

    // Create a string from the buffer
    string content(buffer);

    // Clean up
    fclose(file);
    delete[] buffer;

    // Return the file content as a string
    return content;
  }

  static vector<string> getFilesInDirectory(const string& directoryName) {
    vector<string> filePaths;

    try {
        filesystem::path currentPath = filesystem::current_path();
        filesystem::path directoryPath = currentPath / directoryName;

        for (const auto& entry : filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                filePaths.push_back(entry.path().string());
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error accessing the directory: " << e.what() << endl;
    }

    return filePaths;
}
};