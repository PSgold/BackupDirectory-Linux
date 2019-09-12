#pragma once

#include <fstream>
#include <string>
#include <mutex>

enum class flags { null, open, error };

namespace helperClass {
	
	class log {
	  public:
		log() = default;
		log(std::string filePath);
		~log();

		void setFilePath(std::string);
		void writeLog(std::string);
		flags checkFlag();

	  private:
		std::string filePath;
		std::ofstream logFile;
		flags flag{ flags::null };
		std::mutex fileMutex;
	};

	log::log(std::string filePath) :filePath{ filePath } {
		logFile.open((*this).filePath);
		if (logFile.is_open()) { flag = flags::open; }
		else { flag = flags::error; }
	}

	log::~log() {
		logFile.close();
		if (std::filesystem::is_empty(filePath))std::filesystem::remove(filePath);
	}

	void log::setFilePath(std::string filePath) {
		(*this).filePath = filePath;
		logFile.open((*this).filePath);
		if (logFile.is_open()) { flag = flags::open; }
		else { flag = flags::error; }
	}

	void log::writeLog(std::string content) {
		std::lock_guard<std::mutex> lock(fileMutex);
		logFile << content << "\n";
	}
	flags log::checkFlag() { return flag; }
}