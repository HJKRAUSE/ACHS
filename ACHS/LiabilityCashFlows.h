#pragma once
#include <ql/quantlib.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

namespace ACHS {

	class LiabilityCashFlows {
	public:
		explicit LiabilityCashFlows(const std::string& filename) {
			readFromCSV(filename);
		}

		const QuantLib::Leg& leg() const {
			return cashflows_;
		}

	private:
		QuantLib::Leg cashflows_;

		void readFromCSV(const std::string& filename) {
			std::ifstream file(filename);
			if (!file.is_open()) {
				throw std::runtime_error("LiabilityCashFlows::readFromCSV: failed to open CSV file: " + filename + ".");
			}

			std::string line;
			std::getline(file, line); // skip header

			while (std::getline(file, line)) {
				std::istringstream ss(line);
				std::string token;

				int year, month, day;
				double amount;

				std::getline(ss, token, ','); year = std::stoi(token);
				std::getline(ss, token, ','); month = std::stoi(token);
				std::getline(ss, token, ','); day = std::stoi(token);
				std::getline(ss, token, ','); amount = std::stod(token);

				QuantLib::Date date(day, static_cast<QuantLib::Month>(month), year);
				auto cf = std::make_shared<QuantLib::SimpleCashFlow>(amount, date);
				cashflows_.push_back(cf);
			}
		}
	};

}
