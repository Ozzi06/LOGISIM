#include "file_dialogs.h"
#include <windows.h>

std::string open_file_dialog_json_bin()
{
    // Initialize the OPENFILENAMEA structure
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = new CHAR[MAX_PATH]; // Buffer to store the file name
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Bin Files\0*.bin\0Json Files\0*.json\0\0"; // Filter to specify the extension
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "Saves";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


    // Invoke the file dialog and check if the user selected a file
    if (GetOpenFileNameA(&ofn)) {
        // Convert the file name from CHAR to std::string
        std::string filepath = std::string(ofn.lpstrFile);
        // Delete the buffer
        delete[] ofn.lpstrFile;
        // Return the file path
        return filepath;
    }
    else {
        // Delete the buffer
        delete[] ofn.lpstrFile;
        // Return an empty string
        return "";
    }
}

std::string ShowSaveFileDialogJson()
{
    OPENFILENAMEA ofn;       // Common dialog box structure
    char szFile[260];       // Buffer for file name
    HWND hwnd = NULL;       // Owner window
    HANDLE hf;              // File handle

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0'; // Ensure the file name is initially empty
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Json Files (*.json)\0";
    ofn.nFilterIndex = 1; // Default to showing text files first
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "Saves";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "json"; // Default file extension

    // Display the Save As dialog box
    if (GetSaveFileNameA(&ofn) == TRUE) {
        // Create or open the file
        hf = CreateFileA(ofn.lpstrFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (hf != INVALID_HANDLE_VALUE) {
            CloseHandle(hf); // Close the handle so the caller can use the file
            return std::string(ofn.lpstrFile); // Return the filename
        }
    }

    return ""; // Return an empty string if the dialog is canceled or an error occurs
}
std::string ShowSaveFileDialogBin()
{
    OPENFILENAMEA ofn;       // Common dialog box structure
    char szFile[260];       // Buffer for file name
    HWND hwnd = NULL;       // Owner window
    HANDLE hf;              // File handle

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0'; // Ensure the file name is initially empty
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Binary Files (*.bin)\0";
    ofn.nFilterIndex = 1; // Default to showing text files first
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "Saves";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "bin"; // Default file extension

    // Display the Save As dialog box
    if (GetSaveFileNameA(&ofn) == TRUE) {
        // Create or open the file
        hf = CreateFileA(ofn.lpstrFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (hf != INVALID_HANDLE_VALUE) {
            CloseHandle(hf); // Close the handle so the caller can use the file
            return std::string(ofn.lpstrFile); // Return the filename
        }
    }

    return ""; // Return an empty string if the dialog is canceled or an error occurs
}
