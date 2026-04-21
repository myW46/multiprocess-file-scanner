#pragma once
#include <string>
#include <vector>

#include "json.hpp"


struct Signature
{
	std::string name;
	std::string pattern;
	std::string description;
	std::string mode;
};

class ConfigManager {
public:
	static std::vector<Signature> load(const std::string& filename);

};
