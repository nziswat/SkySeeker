#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "skyseekerTSV.h"
#include <src/json.h>

//for debuggin
void printIcaoData(const icaoData& data) {
	std::cout << "ICAO DATA:" << std::endl;
	std::cout << "\tcountry: " << data.country << std::endl;
	std::cout << "\tregistration: " << data.registration << std::endl;
	std::cout << "\ttypeCode: " << data.typeCode << std::endl;
	std::cout << "\tisMilitary: " << data.isMilitary << std::endl;
}

void fillEmptyData(icaoData& data) {
	if (data.country.empty()) {
		data.country = "UNKNOWN";
	}
	if (data.registration.empty()) {
		data.registration = "UNKNOWN";
	}
	if (data.typeCode.empty()) {
		data.typeCode = "UNKNOWN";
	}
}

void TSV::getDataForICAO(std::string& icao, icaoData& data) {
	std::ifstream file("data/modes.tsv");
	if (!file.is_open()) {
		std::cout << "modes.tsv not found in data dir" << std::endl;
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		size_t tab1 = line.find('\t');
		if (tab1 == std::string::npos) {
			continue;
		}

		size_t tab2 = line.find('\t', tab1 + 1);
		if (tab2 == std::string::npos) {
			continue;
		}

		size_t tab3 = line.find('\t', tab2 + 1);
		std::string token = (tab3 == std::string::npos) ? line.substr(tab2 + 1) : line.substr(tab2 + 1, tab3 - tab2 - 1);

		if (token == icao) {
			size_t tab4 = (tab3 != std::string::npos) ? line.find('\t', tab3 + 1) : std::string::npos;
			data.country = line.substr(0, tab1);
			data.registration = line.substr(tab1 + 1, tab2 - tab1 - 1);
			data.typeCode = token;
			data.isMilitary = (tab3 != std::string::npos && tab4 != std::string::npos) ? (line.substr(tab4 + 1) == "military") : false;
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

//Dear Kevbo, this is how u use this

//void whateverExampleFunction() {
//	std::string icao = "3E6210";
//	icaoData data{};
//	TSV::getDataForICAO(icao, data);
//}