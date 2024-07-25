#include <stdio.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>

// Function to delete a directory and its contents
BOOL DeleteDirectory(const char *dirPath) {
    SHFILEOPSTRUCT fileOp = {
        NULL,
        FO_DELETE,
        dirPath,
        "",
        FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
        FALSE,
        0,
        ""
    };
    return (SHFileOperation(&fileOp) == 0);
}

// Function to delete a registry key and its subkeys
BOOL DeleteRegistryKey(HKEY rootKey, const char *subKey) {
    return (RegDeleteTree(rootKey, subKey) == ERROR_SUCCESS);
}

// Function to delete registry values
BOOL DeleteRegistryValue(HKEY rootKey, const char *subKey, const char *valueName) {
    HKEY hKey;
    if (RegOpenKeyEx(rootKey, subKey, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }
    if (RegDeleteValue(hKey, valueName) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

// Function to delete files from a directory
void DeleteFilesInDirectory(const char *dirPath) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char filePath[MAX_PATH];
    StringCchPrintf(filePath, MAX_PATH, "%s\\*.*", dirPath);

    hFind = FindFirstFile(filePath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            StringCchPrintf(filePath, MAX_PATH, "%s\\%s", dirPath, findFileData.cFileName);
            DeleteFile(filePath);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

// Function to enumerate user profiles
void EnumerateUserProfiles(void (*callback)(const char *)) {
    DWORD size = 0;
    DWORD userCount = 0;
    HANDLE token = NULL;
    PTOKEN_USER ptu = NULL;
    CHAR userPath[MAX_PATH];
    HKEY hKey;
    LSTATUS status;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        GetTokenInformation(token, TokenUser, NULL, 0, &size);
        ptu = (PTOKEN_USER)malloc(size);

        if (ptu && GetTokenInformation(token, TokenUser, ptu, size, &size)) {
            SID_NAME_USE sidType;
            WCHAR name[MAX_PATH], domain[MAX_PATH];
            DWORD nameSize = MAX_PATH, domainSize = MAX_PATH;

            if (LookupAccountSid(NULL, ptu->User.Sid, name, &nameSize, domain, &domainSize, &sidType)) {
                StringCchPrintf(userPath, MAX_PATH, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList");

                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, userPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    WCHAR subKey[MAX_PATH];
                    WCHAR profilePath[MAX_PATH];
                    DWORD subKeySize = MAX_PATH;

                    for (DWORD i = 0; RegEnumKeyEx(hKey, i, subKey, &subKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; i++) {
                        HKEY subHKey;
                        DWORD dataSize = MAX_PATH;

                        if (RegOpenKeyEx(hKey, subKey, 0, KEY_READ, &subHKey) == ERROR_SUCCESS) {
                            if (RegQueryValueEx(subHKey, L"ProfileImagePath", NULL, NULL, (LPBYTE)profilePath, &dataSize) == ERROR_SUCCESS) {
                                WideCharToMultiByte(CP_UTF8, 0, profilePath, -1, userPath, MAX_PATH, NULL, NULL);
                                callback(userPath);
                            }
                            RegCloseKey(subHKey);
                        }
                        subKeySize = MAX_PATH;
                    }
                    RegCloseKey(hKey);
                }
            }
        }
        if (ptu) free(ptu);
        CloseHandle(token);
    }
}

// Callback function to delete Opera-related directories for a given user profile
void CleanupUserProfile(const char *userProfilePath) {
    char path[MAX_PATH];
    const char *userDirsToDelete[] = {
        "\\AppData\\Local\\temp\\Opera",
        "\\AppData\\Local\\Programs\\Opera",
        "\\AppData\\Local\\Opera Software",
        "\\AppData\\Roaming\\Opera Software",
        NULL
    };

    for (int i = 0; userDirsToDelete[i] != NULL; i++) {
        snprintf(path, MAX_PATH, "%s%s", userProfilePath, userDirsToDelete[i]);
        DeleteDirectory(path);
    }

    // Delete Opera shortcuts from Start Menu and Taskbar
    snprintf(path, MAX_PATH, "%s\\AppData\\Roaming\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\Opera.lnk", userProfilePath);
    DeleteFile(path);
    snprintf(path, MAX_PATH, "%s\\AppData\\Roaming\\Microsoft\\Internet Explorer\\Quick Launch\\Opera.lnk", userProfilePath);
    DeleteFile(path);
    snprintf(path, MAX_PATH, "%s\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Opera.lnk", userProfilePath);
    DeleteFile(path);

    // Delete Opera shortcuts from desktop
    snprintf(path, MAX_PATH, "%s\\Desktop\\Opera.lnk", userProfilePath);
    DeleteFile(path);

    // Delete Opera installers from Downloads
    snprintf(path, MAX_PATH, "%s\\Downloads\\Opera*.exe", userProfilePath);
    DeleteFilesInDirectory(path);
}

// Main function
int main() {
    const char *dirsToDelete[] = {
        "C:\\Windows\\temp\\Opera",
        "C:\\Program Files\\Opera",
        "C:\\Program Files (x86)\\Opera",
        NULL
    };

    const struct {
        HKEY rootKey;
        const char *subKey;
    } registryKeysToDelete[] = {
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Opera"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Opera"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\Opera"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run\\Opera"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\RegisteredApplications\\Opera"},
        {HKEY_CURRENT_USER, "SOFTWARE\\RegisteredApplications\\Opera"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Opera Software"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Clients\\StartMenuInternet\\Opera"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Clients\\StartMenuInternet\\Opera"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Classes\\OperaStable"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\OperaStable"},
        {HKEY_CLASSES_ROOT, "OperaStable"},
        NULL
    };

    // Expand environment variables in directory paths and delete directories
    for (int i = 0; dirsToDelete[i] != NULL; i++) {
        char expandedDirPath[MAX_PATH];
        ExpandEnvironmentStrings(dirsToDelete[i], expandedDirPath, MAX_PATH);
        DeleteDirectory(expandedDirPath);
    }

    // Delete registry keys
    for (int i = 0; registryKeysToDelete[i].subKey != NULL; i++) {
        DeleteRegistryKey(registryKeysToDelete[i].rootKey, registryKeysToDelete[i].subKey);
    }

    // Enumerate user profiles and clean up Opera-related files for each user
    EnumerateUserProfiles(CleanupUserProfile);

    printf("Opera cleanup completed.\n");
    return 0;
}
