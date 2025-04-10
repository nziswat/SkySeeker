#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include "skyseekerTSV.h"
#include <src/json.h>
#include "database.h"
const std::string tsvFilename = "data/modes.tsv";
std::vector<indexEntry> tsvIndex; //filled during init

void printIcaoData(const icaoData& data) {
	std::cout << "ICAO DATA:\n";
	std::cout << "\tcountry: " << data.country << "\n";
	std::cout << "\tregistration: " << data.registration << "\n";
	std::cout << "\ttypeCode: " << data.typeCode << "\n";
	std::cout << "\tisMilitary: " << data.isMilitary << "\n";
}

void fillEmptyData(icaoData& data) {
	if (data.country.empty()) data.country = "UNKNOWN";
	if (data.registration.empty()) data.registration = "UNKNOWN";
	if (data.typeCode.empty()) data.typeCode = "UNKNOWN";
}

std::vector<indexEntry> buildLookupTable(size_t step = 1000) {
	std::ifstream file(tsvFilename);
	std::vector<indexEntry> index;

	if (!file.is_open()) {
		return index;
	}

	std::string line;
	size_t counter = 0;

	while (file) {
		std::streampos pos = file.tellg();
		std::getline(file, line);
		if (line.empty()) continue;

		size_t tab1 = line.find('\t');
		if (tab1 == std::string::npos) continue;

		size_t tab2 = line.find('\t', tab1 + 1);
		if (tab2 == std::string::npos) continue;

		size_t tab3 = line.find('\t', tab2 + 1);
		std::string icao = (tab3 == std::string::npos) ? line.substr(tab2 + 1) : line.substr(tab2 + 1, tab3 - tab2 - 1);

		if (counter++ % step == 0) {
			index.push_back({ icao, pos });
		}
	}

	file.close();

	std::cout << "[DEBUG] Lookup table built with " << index.size() << " entries.\n";
	return index;
}

std::vector<std::string> explodeTab(const std::string& input) {
	std::vector<std::string> result;
	std::stringstream ss(input);
	std::string item;

	while (std::getline(ss, item, '\t')) {
		result.push_back(item);
	}

	return result;
}

void TSV::getDataForICAO(std::string& icao, icaoData& data) {
	std::cout << "[DEBUG] Searching for ICAO: " << icao << "\n";

	std::ifstream file(tsvFilename);
	if (!file.is_open()) {
		std::cerr << "[ERROR] modes.tsv not found in data dir\n";
		return;
	}

	//find nearest lower bound in index
	auto it = std::lower_bound(tsvIndex.begin(), tsvIndex.end(), icao,
		[](const indexEntry& entry, const std::string& val) {
			return entry.icao < val;
		});

	if (it != tsvIndex.begin() && (it == tsvIndex.end() || it->icao > icao)) {
		--it;
	}

	std::cout << "[DEBUG] Starting search from ICAO: " << it->icao << " at file position: " << it->pos << "\n";
	file.seekg(it->pos);

	std::string line;
	bool skipped = false;
	while (std::getline(file, line)) {
		std::vector<std::string> exploded = explodeTab(line);
		if (!skipped) {
			std::string token = exploded[0];
			//std::cout << "[!skipped] checking if " << token << " == " << icao << std::endl;
			if (token == icao) {
				data.country = exploded[1];
				data.registration = exploded[2];
				data.typeCode = exploded[3];
				data.isMilitary = exploded[4] == "military" ? 1 : 0;
				break;
			}
			if (token > icao) {
				break;
			}
			skipped = true;
		}
		else {
			std::string token = exploded[2];
			//std::cout << "[skipped] checking if " << token << " == " << icao << std::endl;
			if (token == icao) {
				data.country = exploded[3];
				data.registration = exploded[4];
				data.typeCode = exploded[5];
				data.isMilitary = exploded[6] == "military" ? 1 : 0;
				break;
			}
			if (token > icao) {
				break;
			}
		}
	}

	file.close();
	fillEmptyData(data);
	nlohmann::json j; // struct to json for js handling
	j["country"] = data.country;
	j["registration"] = data.registration;
	j["typeCode"] = data.typeCode;
	j["isMilitary"] = data.isMilitary;
	icao = j.dump(); // just overwrite the handover string lol
}

void TSV::init() {
    tsvIndex = buildLookupTable(100); //very smol step size
}