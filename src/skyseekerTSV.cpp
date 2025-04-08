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
			std::string part = line.substr(tab3 + 1);
			size_t t = part.find('\t');
			data.country = part.substr(0, t);

			size_t t2 = part.find('\t', t + 1);
			data.registration = part.substr(t + 1, t2 - t - 1);

			size_t t3 = part.find('\t', t2 + 1);
			data.typeCode = part.substr(t2 + 1, t3 - t2 - 1);

			size_t t4 = part.find('\t', t3 + 1);
			data.isMilitary = part.substr(t3 + 1, t4 - t3 - t2 - 1) == "military" ? 1 : 0;
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