#pragma once
#include <vector>
#include <string>
#include <queue>
#include <map>

#include "ConfigManager.hpp"

struct ACNode {
	ACNode* next[256];
	ACNode* fail;

	std::vector<std::string> matchedName;
	ACNode():fail(nullptr){
		for (int i = 0; i < 256; i++) next[i] = nullptr;
	}
};

class AhoCorasick
{
public:
	AhoCorasick();
	~AhoCorasick();
	void build(const std::vector<Signature>& signatures);
	std::vector<std::string> search(const std::vector<char>& data);

private:
	ACNode* root;
	bool isBuilt = false;
	std::vector<ACNode*> allNodes;
};
