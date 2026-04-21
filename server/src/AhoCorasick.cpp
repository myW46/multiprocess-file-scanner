#include "AhoCorasick.hpp"

AhoCorasick::AhoCorasick() {
	root = new ACNode();
	allNodes.push_back(root);
}

AhoCorasick::~AhoCorasick() {
	for (ACNode* node : allNodes) {
		delete node;
	}
	allNodes.clear();
}


void AhoCorasick::build(const std::vector<Signature>& signatures) {
	if (isBuilt || signatures.empty()) return;

	for (const auto& sig : signatures) {
		if (sig.pattern.empty()) continue;
		ACNode* curr = root;
		for (unsigned char c : sig.pattern) {
			if (!curr->next[c]) {
				ACNode* newNode = new ACNode();
				allNodes.push_back(newNode);
				curr->next[c] = newNode;
			}
			curr = curr->next[c];
		}
		curr->matchedName.push_back(sig.name);
	}
	//bfs
	std::queue<ACNode*> q;
	for (int i = 0; i < 256; i++) {
		if (root->next[i]) {
			root->next[i]->fail = root;
			q.push(root->next[i]);
		}
		else {
			root->next[i] = root; 
		}
	}

	while (!q.empty()) {
		ACNode* u = q.front();
		q.pop();

		for (int i = 0; i < 256; i++) {
			if (u->next[i] && u->next[i] != root && u->next[i]->fail == nullptr) {
				u->next[i]->fail = u->fail->next[i];


				auto& fMatches = u->next[i]->fail->matchedName;
				if (!fMatches.empty()) {
					u->next[i]->matchedName.insert(u->next[i]->matchedName.end(),
						fMatches.begin(), fMatches.end());
				}
				q.push(u->next[i]);
			}
			else {
				u->next[i] = u->fail->next[i];
			
			}
		}
	}
	isBuilt = true;
}

std::vector<std::string> AhoCorasick::search(const std::vector<char>& data) {
	std::vector<std::string> results;
	ACNode* curr = root;

	for (char c : data) {
		unsigned char uc = static_cast<unsigned char>(c);
		curr = curr->next[uc];

		if (!curr->matchedName.empty()) {
			for (const auto& name : curr->matchedName) {
				results.push_back(name);
			}
		}
	}
	return results;
}