#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "AhoCorasick.hpp"
#include "ConfigManager.hpp"
#include <algorithm>

std::string hexToBytes(std::string hex);

TEST_CASE("Comprehensive Anti-Malware Engine Test") {
    AhoCorasick ac;
    std::vector<Signature> sigs;

    sigs.push_back({ "ReverseShell", "/bin/bash -i", "external access", "text" });
    sigs.push_back({ "ForseRemove", "rm -rf", "deletion risk", "text" });
    sigs.push_back({ "FakeIP", "192.168.1.100", "Control IP", "text" });

    sigs.push_back({ "HexExample", hexToBytes("4889E59090"), "Stack frame + NOPs", "hex" });
    sigs.push_back({ "Null_Threat", hexToBytes("00000000"), "Null bytes", "hex" });
    sigs.push_back({ "EAX_Clear", hexToBytes("31C0"), "XOR EAX, EAX", "hex" });

    ac.build(sigs);

    SUBCASE("Text-based Signatures") {
        std::string content = "Warning: user executed rm -rf / and /bin/bash -i";
        std::vector<char> data(content.begin(), content.end());

        auto results = ac.search(data);

        INFO("Checking for ReverseShell in text stream");
        CHECK(std::find(results.begin(), results.end(), "ReverseShell") != results.end());

        INFO("Checking for ForseRemove in text stream");
        CHECK(std::find(results.begin(), results.end(), "ForseRemove") != results.end());
    }

    SUBCASE("IP Address Detection") {
        std::string content = "Connecting to 192.168.1.100... Established.";
        std::vector<char> data(content.begin(), content.end());

        auto results = ac.search(data);

        INFO("Scanning for FakeIP signature");
        CHECK(std::find(results.begin(), results.end(), "FakeIP") != results.end());
    }

    SUBCASE("Binary Hex Signatures") {
        std::vector<char> data = {
            0x00, 0x11,
            static_cast<char>(0x48), static_cast<char>(0x89), static_cast<char>(0xE5),
            static_cast<char>(0x90), static_cast<char>(0x90),
            0x22
        };

        auto results = ac.search(data);

        INFO("Checking for HexExample (complex binary pattern)");
        CHECK(std::find(results.begin(), results.end(), "HexExample") != results.end());
    }

    SUBCASE("Multiple Null Bytes (Zero-check)") {
        std::vector<char> data = { 'A', 0x00, 0x00, 0x00, 0x00, 'Z' };

        auto results = ac.search(data);

        INFO("Scanning for Null_Threat (sequence of zeros)");
        CHECK(std::find(results.begin(), results.end(), "Null_Threat") != results.end());
    }

    SUBCASE("Overlapping Patterns (Aho-Corasick Core)") {
        std::vector<char> data = { 0x55, 0x31, static_cast<char>(0xC0), 0x5D };

        auto results = ac.search(data);

        INFO("Checking for EAX_Clear inside larger binary block");
        CHECK(std::find(results.begin(), results.end(), "EAX_Clear") != results.end());
    }

    SUBCASE("Negative Case: Clean Data") {
        std::string content = "Hello world! This is a clean file.";
        std::vector<char> data(content.begin(), content.end());

        auto results = ac.search(data);

        INFO("Ensuring no false positives in clean data");
        CHECK(results.empty());
    }
}