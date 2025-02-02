#include "file_dialogs.h"
#ifdef _WIN32
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
std::string open_file_dialog_hex()
{
    // Initialize the OPENFILENAMEA structure
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = new CHAR[MAX_PATH]; // Buffer to store the file name
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "HEX data Files\0*.hex\0\0"; // Filter to specify the extension
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
#elif __linux__
#include <gtk/gtk.h>
#include <string>

// Helper: Initialize GTK if it hasn’t been already.
static void ensure_gtk_initialized()
{
    static bool initialized = false;
    if (!initialized) {
        int argc = 0;
        char **argv = nullptr;
        // Use gtk_init_check so that in case initialization fails we don’t abort.
        if (!gtk_init_check(&argc, &argv)) {
            g_warning("Failed to initialize GTK.");
        }
        initialized = true;
    }
}

/// Opens a file dialog that allows selection of .bin or .json files.
/// Returns the selected file path or an empty string if cancelled.
std::string open_file_dialog_json_bin()
{
    ensure_gtk_initialized();

    // Create an "Open File" dialog.
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // Set the initial folder to "Saves" if possible.
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "Saves");

    // Create and add a filter for binary (*.bin) files.
    GtkFileFilter *filter_bin = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_bin, "Bin Files");
    gtk_file_filter_add_pattern(filter_bin, "*.bin");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_bin);

    // Create and add a filter for JSON (*.json) files.
    GtkFileFilter *filter_json = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_json, "Json Files");
    gtk_file_filter_add_pattern(filter_json, "*.json");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_json);

    std::string filename;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (file) {
            filename = file;
            g_free(file);
        }
    }
    gtk_widget_destroy(dialog);

    // Process any pending GTK events to clean up properly.
    while (gtk_events_pending())
        gtk_main_iteration();
    
    return filename;
}

/// Opens a file dialog that allows selection of a .hex file.
/// Returns the selected file path or an empty string if cancelled.
std::string open_file_dialog_hex()
{
    ensure_gtk_initialized();

    // Create an "Open File" dialog.
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open HEX File",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // Set the initial folder to "Saves".
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "Saves");

    // Create and add a filter for HEX (*.hex) files.
    GtkFileFilter *filter_hex = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_hex, "HEX Data Files");
    gtk_file_filter_add_pattern(filter_hex, "*.hex");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_hex);

    std::string filename;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (file) {
            filename = file;
            g_free(file);
        }
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration();
    
    return filename;
}

/// Opens a "Save File" dialog for a JSON file.
/// If the user accepts, it creates (or truncates) the file and returns its path.
/// Returns an empty string if cancelled.
std::string ShowSaveFileDialogJson()
{
    ensure_gtk_initialized();

    // Create a "Save File" dialog.
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save JSON File",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // Set initial folder to "Saves".
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "Saves");

    // Enable overwrite confirmation.
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    // Set a default filename (optional).
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "untitled.json");

    // Add a filter for JSON files.
    GtkFileFilter *filter_json = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_json, "Json Files");
    gtk_file_filter_add_pattern(filter_json, "*.json");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_json);

    std::string filename;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (file) {
            filename = file;
            g_free(file);

            // Optionally, create/truncate the file.
            FILE *fp = fopen(filename.c_str(), "wb");
            if (fp) {
                fclose(fp);
            } else {
                g_warning("Could not create file: %s", filename.c_str());
                filename.clear();
            }
        }
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration();
    
    return filename;
}

/// Opens a "Save File" dialog for a binary file.
/// If the user accepts, it creates (or truncates) the file and returns its path.
/// Returns an empty string if cancelled.
std::string ShowSaveFileDialogBin()
{
    ensure_gtk_initialized();

    // Create a "Save File" dialog.
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Binary File",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // Set initial folder to "Saves".
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "Saves");

    // Enable overwrite confirmation.
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    // Set a default filename (optional).
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "untitled.bin");

    // Add a filter for binary files.
    GtkFileFilter *filter_bin = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_bin, "Binary Files");
    gtk_file_filter_add_pattern(filter_bin, "*.bin");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_bin);

    std::string filename;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (file) {
            filename = file;
            g_free(file);

            // Optionally, create/truncate the file.
            FILE *fp = fopen(filename.c_str(), "wb");
            if (fp) {
                fclose(fp);
            } else {
                g_warning("Could not create file: %s", filename.c_str());
                filename.clear();
            }
        }
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration();
    
    return filename;
}
#endif
