#include "EagleVMStub/EagleVMStub.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <array>

// xorstr implementation
constexpr std::uint32_t xorstr_key = 0xAB;

constexpr char xorstr(char c, std::size_t i) {
    return c ^ (xorstr_key + i);
}

#define xorstr_(str) []() { \
    std::array<char, sizeof(str)> encrypted = {}; \
    for (std::size_t i = 0; i < sizeof(str); ++i) \
        encrypted[i] = xorstr(str[i], i); \
    return encrypted; \
}()

// Custom encryption and decryption functions
std::string decrypt(const std::array<char, sizeof("12345678901234567890")>& encrypted) {

    // Begin our virtualization
    fnEagleVMBegin();

    std::string decrypted = "";
    for (std::size_t i = 0; i < encrypted.size(); ++i) {
        decrypted += xorstr(encrypted[i], i);
    }

    // End our virtualization
    fnEagleVMEnd();

    return decrypted;
}

std::array<char, sizeof("S-1-5-18")> encryptedSID = xorstr_("S-1-5-18");

std::string hiddenLicense;

// Function to get SID of the machine
std::string getSID() {

    // Begin our virtualization
    fnEagleVMBegin();

    PSID sid = nullptr;
    char* sidString = nullptr;
    std::string sidStringResult = "";

    if (ConvertStringSidToSidA(decrypt(encryptedSID).c_str(), &sid)) {
        if (ConvertSidToStringSidA(sid, &sidString)) {
            sidStringResult = sidString;
            LocalFree(sidString);
        }
        LocalFree(sid);
    }

    // End our virtualization
    fnEagleVMEnd();

    return sidStringResult;
}

// Function to generate a hidden license key combining SID
std::string generateHiddenLicense(const std::string& sid) {

    // Begin our virtualization
    fnEagleVMBegin();

    // Use current time as seed for random generator
    srand(time(0));

    // Generate a random string for the license
    std::string license = "";
    for (int i = 0; i < 16; ++i) {
        char c = 'A' + rand() % 26;
        license += c;
    }

    // Combine SID with license
    license += "-" + sid;

    // Shuffle the license string
    std::random_shuffle(license.begin(), license.end());

    // End our virtualization
    fnEagleVMEnd();

    return license;
}

// Function to extract and decrypt SID from the license key
std::string extractAndDecryptSID(const std::string& licenseKey) {

    // Begin our virtualization
    fnEagleVMBegin();

    // Parse the license key to extract and decrypt SID
    std::stringstream ss(licenseKey);
    std::string token;
    while (std::getline(ss, token, '-')) {
        // Assuming SID is after the '-' delimiter
        if (!token.empty()) {
            return token; // No need to decrypt SID since it's already decrypted at compile time
        }
    }

    // End our virtualization
    fnEagleVMEnd();

    return "";
}

// Function to verify the license key
bool verifyLicense(const std::string& licenseKey) {

    // Begin our virtualization
    fnEagleVMBegin();
    
    // Extract and decrypt SID from license key
    std::string extractedSID = extractAndDecryptSID(licenseKey);

    // Generate SID from current system
    std::string currentSID = getSID();

    // Compare the extracted and current SIDs
    if (extractedSID != currentSID) {
        return false;
    }

    // Compare the hidden and inputted keys
    if (hiddenLicense != licenseKey) {
        return false;
    }

    // End our virtualization
    fnEagleVMEnd();

    return true;
}

int main() {

    // Begin our virtualization
    fnEagleVMBegin();

    std::cout << decrypt(xorstr_("Welcome to the EagleVM Sandbox!\n"));

    // Generate SID
    std::string sid = getSID();

    // Generate hidden license key combining encrypted SID
    hiddenLicense = generateHiddenLicense(sid);
    std::cout << "Generated License Key: " << hiddenLicense << std::endl;

    // Main loop for license verification
    while (true) {
        std::cout << decrypt(xorstr_("Enter license key: "));
        std::string licenseKey;
        std::cin >> licenseKey;

        if (verifyLicense(licenseKey)) {
            std::cout << decrypt(xorstr_("License verified successfully!\n"));
            break;
        } else {
            std::cout << decrypt(xorstr_("Invalid license key. Please try again.\n"));
        }
    }

    system(xorstr_("pause"));

    // End our virtualization
    fnEagleVMEnd();

    return 0;
}
