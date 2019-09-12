// BackupDirectory.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iomanip>
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <ctime>
#include <cstring>
#include <fcntl.h>
#include "helperClass.h"
#include "fileObj.h"
#include "BDcopy.h"

//struct obj used to keep status of source and dest path status
struct pathFlags {
	bool source{0};
	bool dest{0};
};
pathFlags pathFlag;
bool logFileState{ 0 };//0 closed, 1 open

//local functions
void displayPreamble();
wchar_t displayMenu();
std::string getSourceDir();
std::string getDestDir();
void pause(char);
bool setTempPathStr(std::string&);

class logFileError{
public: void printError(){std::cout << "Failed to open log file. Aborting program."<<std::endl;}
};

int main(int argc, char* argv[]) {
	
	
	//If it was run via terminal with 2 params source and dest ======================================================================================================
	if (argc > 1) {
		std::string fileName{"/tmp/BackUpDirectoryLog_"};
		setTempPathStr(fileName);
		helperClass::log* logFilePtr{ new helperClass::log { fileName } };
		
		std::string logTxt{ "Source path entered: \"" };
		logTxt += argv[1]; logTxt += "\"";
		logFilePtr->writeLog(logTxt);
		std::string logTxt2{ "Destination path entered: \"" };
		logTxt2 += argv[2]; logTxt2 += "\"";
		logFilePtr->writeLog(logTxt2);
		
		int sourceSize = std::char_traits<char>::length (argv[1]);
		int destSize = std::char_traits<char>::length(argv[2]);
		try {
			for (int c{ 0 }; c <= sourceSize; c++) {
				if (argv[1][c] == '"') throw 798;
			}
			for (int c{ 0 }; c <= destSize; c++) {
				if (argv[2][c] == '"') throw 799;
			}
		}
		catch(int ex){
			delete logFilePtr;
			switch (ex){
			case 798:std::cout << "Error: source path cannot have single trailing backslash '\\'. "
				<< "Either use a double trailing backslash '\\\\' or delete the trailing backslash."; break;
			case 799:std::cout << "Error: destination path cannot have single trailing backslash '\\'. "
				<< "Either use a double trailing backslash '\\\\' or delete the trailing backslash."; break;
			}
			return 0;
		}
		fs::path source{ argv[1] };
		std::string dest{ argv[2] };
		logFilePtr->writeLog("\nstartBackup has been called...");
		startBackup(source, dest, logFilePtr);
		delete logFilePtr; return 0;
	}
	//If it was run via terminal with 2 params source and dest =======================================================================================================
	
	
	
	//Creating source and destination Directory Variables used in main loop
	std::filesystem::path source;
	std::filesystem::path destination;
	std::string sourceStr;
	std::string destStr;
	wchar_t choice;


	//displays to console introduction explaining what the program does
	displayPreamble(); pause('M');

	//create pointer to log file. This will allow us to open and close log files per our need
	//only one log file will be open at a time
	helperClass::log* logFilePtr{ nullptr };
	
	//Start Main loop
	while (true) {
		if (!logFileState) {
			std::string fileName{"/tmp/BackUpDirectoryLog_"};
			if (!setTempPathStr(fileName)) {
				std::cout << "Failed to load temp path environment variable. Aborting Program."
					<< std::endl; pause('Q'); return 0;
			}
			else{
				//Creating log file obj with file path name and testing if its open for writing
				logFilePtr = new helperClass::log { fileName };
				try { if (logFilePtr->checkFlag() != flags::open) { throw logFileError{}; } }
				catch (logFileError& ex) { ex.printError(); delete logFilePtr; pause('Q'); return 0; }
				logFileState = 1;
			}
		}
		choice = displayMenu();
		if (choice == '1') {
			sourceStr = getSourceDir();
			std::string logTxt{ "Source path entered: \"" };
			logTxt += sourceStr; logTxt += "\"";
			logFilePtr->writeLog(logTxt);
			source = sourceStr;
			try { 
				if (!(std::filesystem::exists(source))) throw 0;
				pathFlag.source = 1;
				logFilePtr->writeLog("Source path exists!");
				std::cout << "Source path has been set to \""<<sourceStr<<"\"\n";
				pause('M');
			}
			catch (int& ex) {
				pathFlag.source = 0;
				std::string logTxt{ "Source path \"" };
				logTxt += sourceStr; logTxt += "\" does not exist";
				logFilePtr->writeLog(logTxt);
				std::cout << "Error: source directory does not exist! Source has been cleared\n";
				pause('M');
			}
			logFilePtr->writeLog("");
			system("clear");
		}
		else if (choice == '2') {
			destStr = getDestDir();
			std::string logTxt{ "Destination path entered: " };
			logTxt += destStr;
			logFilePtr->writeLog(logTxt);
			destination = destStr;
			try { 
				if (!(std::filesystem::exists(destination))) throw 0; 
				pathFlag.dest = 1;
				logFilePtr->writeLog("Destination path exists!");
				std::cout << "Destination path has been set to \"" << destStr << "\"\n";
				pause('M');
			}
			catch (int& ex) {
				pathFlag.dest = 0;
				std::string logTxt{ "Destination path \"" };
				logTxt += destStr; logTxt += "\" does not exist";
				logFilePtr->writeLog(logTxt);
				std::cout << "Error: destination directory does not exist! Destination has been cleared\n";
				pause('M');
			}
			logFilePtr->writeLog("");
			system("clear");
		}
		else if (choice == '3') {
			if (pathFlag.source == 1)std::cout << "Source path is set to \"" << sourceStr << "\"\n\n";
			else std::cout << "Source path has not been set\n\n";
			pause('M');
		}
		else if (choice == '4') {
			if (pathFlag.dest == 1)std::cout << "Destination path is set to \"" << destStr << "\"\n\n";
			else std::cout << "Destination path has not been set\n\n";
			pause('M');
		}
		else if (choice == '5') {
			if(pathFlag.source == 1 && pathFlag.dest==1){
				pathFlag.source = 0; pathFlag.dest = 0;
				logFilePtr->writeLog("\nstartBackup has been called...");
				startBackup(source,destStr,logFilePtr);
				std::cout << "\n\nBackup has finished...\n\n";
				pause('Q'); delete logFilePtr;logFilePtr = nullptr; logFileState = 0;
				
			}
			else if (pathFlag.source == 0 && pathFlag.dest == 0) {
				logFilePtr->writeLog("startBackup was called but source and destination path have not been set");
				std::cout << "Source and destination path have not been set\n\n";
				pause('M');
			}
			else if (pathFlag.source==0){
				logFilePtr->writeLog("startBackup was called but source path has not been set");
				std::cout << "Source path has not been set\n\n";
				pause('M');
			}
			else if (pathFlag.dest == 0) {
				logFilePtr->writeLog("startBackup was called but destination path has not been set");
				std::cout << "Destination path has not been set\n\n";
				pause('M');
			}
		}
		else if (choice == L'6') { pause('Q'); delete logFilePtr; return 0; }
	}
	//End Main loop
	
	delete logFilePtr; return 0;//unnecessary
}

void displayPreamble() {
	std::cout << std::setw(8) << "Preamble" << std::endl;
	std::cout << std::setw(8) << std::setfill('=') << "" << std::endl;
	std::cout << "This program will copy the source directory tree and files"
		<< " to the destination directory.\nIt will not remove any existing"
		<< " destination directories, rather will only create new ones if they don't already exist.\n"
		<< "If a file already exists in the the same place at the destination"
		<< " it will only be truncated if the source file's modified time"
		<< " is newer than the destination file's modified time." << std::endl << std::endl;
}

wchar_t displayMenu() {
	char choice;
	std::string temp;
	std::istringstream tempStream{"0"};
	int counter{ 0 };
	system("clear");
	while (tempStream.str() != "1" && tempStream.str() != "2" && 
		   tempStream.str() != "3" && tempStream.str() != "4" &&
		   tempStream.str() != "5" && tempStream.str() != "6") {
		if (counter > 0) { std::cout <<"'"<< temp <<"'"<< " is not a valid choice. Please choose again.\n\n"; }
		std::cout << "===========Menu===========" << std::endl<<std::endl;
		std::cout << "1. Enter source directory\n2. Enter destination directory"
			<<"\n3. Check source directory\n4. Check destination directory"
			<<"\n5. Start backup\n6. Quit\n\n";
		std::cout << std::setw(27) << std::setfill('=') << "" << std::endl;
		std::cout << "Please choose (1,2,3,4,5,6): ";
		std::getline(std::cin, temp);
		tempStream.str(temp);
		system("clear");
		counter++;
	}
	tempStream >> choice; return choice;
}

std::string getSourceDir() {
	std::string tempSource;
	std::cout << "Please enter the full source directory path: ";
	std::getline(std::cin, tempSource); return tempSource;
}

std::string getDestDir() {
	std::string tempSource;
	std::cout << "Please enter the full destination directory path: ";
	std::getline(std::cin, tempSource); return tempSource;
}

void pause(char pauseType) {
	std::string tmp;
	if(pauseType == 'Q')std::cout << "Press Enter to quit... ";
	else if (pauseType == 'M')std::cout << "Press Enter for menu... ";
	std::getline(std::cin, tmp);
}

bool setTempPathStr(std::string& fileName) {
	//Get system time, create filepathname with systime in temp folder and create and open log file
	std::time_t sysTime;
	std::time(&sysTime);
	std::tm* resultPtr{localtime(&sysTime)};//puts sysTime into tm obj which holds time as calendar time
	//places the tm obj with specific format into string buff, stringBuffO;fails if you try to put it directly into wostringstream 
	std::ostringstream stringBuffO;
	stringBuffO << std::put_time(resultPtr, "%d.%m.%y_%H.%M.%S");
	std::string stringBuffS{ stringBuffO.str() };

	//creating full file path and name
	fileName += stringBuffO.str();
	fileName += ".txt";
	return 1;
}


//Another way of copying a file with C++ using your own buffer:
//inputfile.seekg(0, inputfile.end);
//int length = inputfile.tellg();
//inputfile.seekg(0, inputfile.beg);
//char* buffer = new char[length];
//inputfile.read(buffer, length);
//outputfile.write(buffer, length);
//delete[] buffer;


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
