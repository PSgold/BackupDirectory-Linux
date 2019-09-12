#pragma once

#include <iostream>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <thread>
#include "helperClass.h"
#include "fileObj.h"

struct errorType {
	std::string file;
	std::string errorStr;
	errorType(std::string file, std::string errorStr)
		:file{ file }, errorStr{ errorStr }{}
};

//class purpose is to wrap the vector of error objs so two threads 
//can safely access the vector 
class errorVecWrapper {
	std::mutex m;
	std::vector <errorType> fsErrorVec{};

public:
	void pushToVec(errorType& fsError) {
		std::lock_guard<std::mutex> lock(m);
		fsErrorVec.push_back(fsError);
	}
	std::vector<errorType>& getErrorVec() {
		return fsErrorVec;
	}
};
std::mutex coutMutex;

class printLock {
	std::mutex m;
	std::string path1;
	std::string path2;
	unsigned cFCount;//current file count to be updated by both threads
	unsigned tFCount;//total file count
	
	public:
		printLock(unsigned tFCount) : tFCount{ tFCount }, cFCount{ 0 },path1{""},path2{""}{}
	void print(std::string path,bool thread) {
		std::lock_guard<std::mutex> lockG(coutMutex);
		std::lock_guard<std::mutex> lock(m);
		(!thread) ? (path1 = path) : (path2 = path);
		
		std::cout<<"\033[s";
		std::cout << "Checking\\Backing up file "
			<< cFCount << "/" << tFCount << '\n';
		std::cout << "Thread 0 current file: " << path1<<'\n'
			<< "Thread 1 current file: " << path2;std::cout.flush();
		usleep(100000);
		if (cFCount!=tFCount){
			std::cout<<"\033[u";
			std::cout<<"\033[J";
		}
	}
	void addToFCount() { 
		std::lock_guard<std::mutex> lock(m);
		cFCount++; 
	}
};

//will be called by child thread to copy file to dest
//if file doesn't exist or if source file modified date is newer than dest file
void copy1(
	std::vector<fileObj>::reverse_iterator fileVecIt, const int size,
	helperClass::log* logFile, errorVecWrapper*, printLock*
);

//called by main thread. does the same as copy1
void copy2(
	std::vector<fileObj>::reverse_iterator& fileVecIt, const int size,
	helperClass::log* logFile, errorVecWrapper&, printLock&
);

bool copyTry2(std::vector<fileObj>::reverse_iterator&);

//returns the part of source path that will be added to dest path
std::string getEOP(const std::string& path, const size_t& count);

//startBackup takes the original sourcePath as a path obj, 
//the original dest path as std::string and a pointer to the logfile obj
int startBackup(const fs::path& sourceDir, std::string dest, helperClass::log* logFilePtr) {
	if (dest.back() == L'\\')dest.pop_back();//get rid of end slash in dest path if exists. not necessary with source as it was passed as path obj.
	
	fs::recursive_directory_iterator sourceFileC(sourceDir);
	unsigned fileCount{0};
	for (sourceFileC; sourceFileC != fs::end(sourceFileC); sourceFileC++) {
		if (!(fs::is_directory((*sourceFileC).path()))) fileCount++;
	}

	fs::recursive_directory_iterator sourceDirIt(sourceDir);//iterartor to source obj that will iterate to all files and dirs in source recursively
	std::unique_ptr<std::vector<fileObj>> fileVecPtr{ 
		new std::vector<fileObj>(fileCount)
	};//creates vector to hold file objs on heap
	unsigned vecPointer{ 0 };
	errorVecWrapper EVW{}; errorVecWrapper* EVWPtr{ &EVW }; 
	for (sourceDirIt; sourceDirIt != fs::end(sourceDirIt); sourceDirIt++) {
		if (fs::is_directory((*sourceDirIt).path())) {
			std::string tempDest{ dest };
			std::string tempSource{ (*sourceDirIt).path().string() };
			tempDest += "/";
			tempDest += getEOP(tempSource, sourceDir.string().size());
			fs::path destSubDir{ tempDest };
			if (fs::exists(destSubDir) && fs::is_directory(destSubDir));
			else {
				fs::create_directories(destSubDir);
				std::string tempWrite{ "Directory Created: " };
				tempWrite += destSubDir;
				logFilePtr->writeLog(tempWrite);
				std::cout << tempWrite << std::endl;
			}
		}
		else {
			std::string tempDest{ dest };
			std::string tempSource{ (*sourceDirIt).path().string() };
			tempDest += "/";
			tempDest += getEOP(tempSource, sourceDir.string().size());//source.size()
			fs::path destSubDir{ tempDest };
			if (fs::exists(destSubDir)) {
				fileObj temp{
					(*sourceDirIt).path(), destSubDir,
					fs::file_size((*sourceDirIt).path()),
					fs::last_write_time((*sourceDirIt).path()),
					fs::last_write_time(destSubDir)
				};
				(*fileVecPtr)[vecPointer] = temp; vecPointer++;
			}
			else {
				fileObj temp{
					(*sourceDirIt).path(), destSubDir,
					fs::file_size((*sourceDirIt).path()),
					fs::last_write_time((*sourceDirIt).path()),0
				};
				(*fileVecPtr)[vecPointer] = temp; vecPointer++;
			}
		}
	}
	std::cout << "\n\n";
	std::sort(fileVecPtr->begin(), fileVecPtr->end());
	std::vector<fileObj>::reverse_iterator fileVecIt{ fileVecPtr->rbegin() };
	//int vecSize{ static_cast<int>(fileVecPtr->size()) };
	std::cout << "Source has " << fileCount << " files: checking and backing up when necessary."
		<< "\nPlease be patient...\n\n";
	printLock print{ fileCount }; printLock* printPtr{ &print };
	logFilePtr->writeLog("\nFiles created\\updated:");
	std::thread thread2{ copy1,fileVecIt, fileCount, logFilePtr,EVWPtr,printPtr };
	copy2(fileVecIt, fileCount, logFilePtr, EVW,print);
	thread2.join();
	
	std::cout<<std::endl;
	if (!((EVW.getErrorVec()).empty())) {
		logFilePtr->writeLog("\n");
		logFilePtr->writeLog("Error checking\\copying the follwing files:");
		std::vector<errorType>tempVec{ EVW.getErrorVec() };
		for (int c{ 0 }; c < EVW.getErrorVec().size(); c++) {
			std::string temp{ EVW.getErrorVec().at(c).file };
			temp += u8" - "; temp += EVW.getErrorVec().at(c).errorStr;
			logFilePtr->writeLog(temp);
		}
		std::cout << "\n\n(Error checking\\copying certain files. Please check log for more information)";
	}
	return 1;
}

std::string getEOP(const std::string& path, const size_t& count) {
	std::string pathPart2{};
	for (size_t c{ 0 }; c <= path.size(); c++) {
		if (c > (count - 1))pathPart2 += path[c];
	}
	return pathPart2;
}

void copy1(std::vector<fileObj>::reverse_iterator fileVecIt, const int size,
	helperClass::log* logFile, errorVecWrapper* EVW,printLock* printPtr
) {
	int sizeHalf;
	if (size % 2 == 0)sizeHalf = size / 2;
	else sizeHalf = (size / 2) + 1;

	for (int c{ 0 }; c < sizeHalf; c++) {
		printPtr->addToFCount(); printPtr->print(fileVecIt->sourcePath.string(),0);
		try {
			if (fs::copy_file(fileVecIt->sourcePath, fileVecIt->destPath, fs::copy_options::update_existing)) {
				std::string logStrW{ "\"" };
				logStrW += fileVecIt->destPath.string();
				logStrW.pop_back(); logStrW += "\"";logStrW+="T1";
				(*logFile).writeLog(logStrW);
			}
		}
		catch (std::filesystem::filesystem_error& fsError) {
			try {
				if (copyTry2(fileVecIt)) {
					std::string logStrW{ "\"" };
					logStrW += fileVecIt->destPath.string();
					logStrW.pop_back(); logStrW += "\"";
					logFile->writeLog(logStrW);
				}
			}
			catch (errorType& error) { EVW->pushToVec(error); }
		}
		if (c != sizeHalf - 1)fileVecIt += 2;
	}
}

void copy2(std::vector<fileObj>::reverse_iterator& fileVecIt, const int size,
	helperClass::log* logFile, errorVecWrapper& EVW,printLock& print
) {
	int sizeHalf{ size / 2 };
	fileVecIt++;
	for (int c{ 0 }; c < sizeHalf; c++) {
		print.addToFCount(); print.print(fileVecIt->sourcePath.string(),1);
		try {
			if (fs::copy_file(fileVecIt->sourcePath, fileVecIt->destPath, fs::copy_options::update_existing)) {
				std::string logStrW{ "\"" };
				logStrW += fileVecIt->destPath.string();
				logStrW.pop_back(); logStrW += "\"";logStrW+="T0";
				logFile->writeLog(logStrW);
			}
		}
		catch (std::filesystem::filesystem_error& fsError) {
			try {
				if (copyTry2(fileVecIt)) {
					std::string logStrW{ "\"" };
					logStrW += fileVecIt->destPath.string();
					logStrW.pop_back(); logStrW += "\"";
					logFile->writeLog(logStrW);
				}
			}
			catch (errorType& error) { EVW.pushToVec(error); }
		}
		if (c != sizeHalf - 1)fileVecIt += 2;
	}
}

bool copyTry2(std::vector<fileObj>::reverse_iterator& fileVecIt) {
	if (fileVecIt->destPathExists) {
		if (fileVecIt->lastWriteTimeSource > fileVecIt->lastWriteTimeDest) {
			std::ifstream source{ fileVecIt->sourcePath,std::ios::binary };
			std::ofstream dest{ fileVecIt->destPath,std::ios::binary | std::ios::trunc };

			if (!source.is_open()||!dest.is_open()) {
				source.close(); dest.close();
				errorType error{
					fileVecIt->sourcePath.u8string(),
					u8"fs::copy_file and fstream open both failed!"
				}; throw error;
			}
			dest << source.rdbuf();
			source.close(); dest.close();
			return 1;
		}
		else return 0;
	}
	else {
		std::ifstream source{ fileVecIt->sourcePath,std::ios::binary };
		std::ofstream dest{ fileVecIt->destPath,std::ios::binary };

		if (!source.is_open() || !dest.is_open()) {
			source.close(); dest.close();
			errorType error{
				fileVecIt->sourcePath.u8string(),
				u8"fs::copy_file and fstream open both failed!"
			}; throw error;
		}
		dest << source.rdbuf();
		source.close(); dest.close();
		return 1;
	}
}