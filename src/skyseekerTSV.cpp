#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include "skyseekerTSV.h"
#include <src/json.h>

const std::string filename = "data/modes.tsv";
std::vector<indexEntry> indexLUT;

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
	//std::cout << "[DEBUG] Building lookup table from file: " << filename << "\n";

	std::ifstream file(filename);
	std::vector<indexEntry> indexTbl;

	if (!file.is_open()) {
		//std::cerr << "[ERROR] Failed to open " << filename << "\n";
		return indexTbl;
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
			indexTbl.push_back({ icao, pos });
		}
	}

	file.close();

	//std::cout << "[DEBUG] Lookup table built with " << index.size() << " entries.\n";
	return indexTbl;
}

void TSV::getDataForICAO(std::string& icao, icaoData& data) {
	//std::cout << "[DEBUG] Searching for ICAO: " << icao << "\n";

	std::ifstream file(filename);
	if (!file.is_open()) {
		//std::cerr << "[ERROR] modes.tsv not found in data dir\n";
		return;
	}

	//find nearest lower bound in index
	auto it = std::lower_bound(indexLUT.begin(), indexLUT.end(), icao,
		[](const indexEntry& entry, const std::string& val) {
			return entry.icao < val;
		});

	if (it != indexLUT.begin() && (it == indexLUT.end() || it->icao > icao)) {
		--it;
	}

	//std::cout << "[DEBUG] Starting search from ICAO: " << it->icao << " at file position: " << it->pos << "\n";
	file.seekg(it->pos);

	std::string line;
	while (std::getline(file, line)) {
		size_t tab1 = line.find('\t');
		if (tab1 == std::string::npos) continue;

		size_t tab2 = line.find('\t', tab1 + 1);
		if (tab2 == std::string::npos) continue;

		size_t tab3 = line.find('\t', tab2 + 1);
		std::string token = (tab3 == std::string::npos) ? line.substr(tab2 + 1) : line.substr(tab2 + 1, tab3 - tab2 - 1);

		if (token == icao) {
			std::string part = line.substr(tab3 + 1);
			size_t t = part.find('\t');
			data.country = part.substr(0, t);

			size_t t2 = part.find('\t', t + 1);
			data.registration = part.substr(t + 1, t2 - t - 1);

			size_t t3 = part.find('\t', t2 + 1);
			data.typeCode = part.substr(t2 + 1, t3 - t2 - 1);

			size_t t4 = part.find('\t', t3 + 1);
			data.isMilitary = part.substr(t3 + 1, t4 - t3 - t2 - 1) == "military" ? 1 : 0;

			//std::cout << "[DEBUG] ICAO match found.\n";
			break;
		}

		if (token > icao) {
			//std::cout << "[DEBUG] Stopped search: passed ICAO range.\n";
			break;
		}
	}

	file.close();
	fillEmptyData(data); // replace all with empty
	nlohmann::json j; // struct to json for js handling
	j["country"] = data.country;
	j["registration"] = data.registration;
	j["typeCode"] = data.typeCode;
	j["isMilitary"] = data.isMilitary;
	icao = j.dump(); // just overwrite the handover string lol
}

void TSV::init() {
	indexLUT = buildLookupTable(1000); //every 1000 lines seems good

	//std::string icao = "EFFFED";
	//icaoData data{};
	//getDataForICAO(icao, data);

	//printIcaoData(data);
}
