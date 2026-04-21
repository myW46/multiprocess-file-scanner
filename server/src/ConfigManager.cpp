#include "ConfigManager.hpp"
#include <fstream>
#include <stdexcept>


using json = nlohmann::json;

std::string hexToBytes(std::string hex) {

	hex.erase(std::remove(hex.begin(), hex.end(), ' '), hex.end());

	std::string bytes;
	for (size_t i = 0; i < hex.length(); i += 2) {
		if (i + 1 < hex.length()) {
			std::string byteString = hex.substr(i, 2);
			unsigned char byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));
			bytes.push_back(static_cast<char>(byte));
		}
	}
	return bytes;
}

std::vector<Signature> ConfigManager::load(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open config file: " + filename);
	}
	json j;
	file >> j;
	std::vector<Signature> viruses;
	if (j.contains("types") && j["types"].is_array()) {
		for (const auto& item : j["types"]) {
			Signature sig;
			sig.name = item.value("name", "Unknown");
			sig.description = item.value("description", "-");
			sig.mode = item.value("mode", "text"); 

			std::string rawPattern = item.value("pattern", "");

			if (sig.mode == "hex") {
				sig.pattern = hexToBytes(rawPattern);
			}
			else {
				sig.pattern = rawPattern;
			}

			viruses.push_back(sig);
		
		}
	}
	return viruses;
}
