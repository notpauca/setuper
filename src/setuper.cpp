#include <iostream>
#include <curl/curl.h>
#include <sys/stat.h>
#include <filesystem>
#include "OpenDMG.hpp"
#include "HFSOperations.h"
#include <thread>

const std::string Agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.82 Safari/537.36";

size_t WriteCallback(char *contents, size_t size, size_t nmemb, FILE *filestream)
{
	size_t res = fwrite(contents, size, nmemb, filestream);
	return res; 
}

int request(std::string Address, std::string outputName) {
	curl_global_init(CURL_GLOBAL_ALL); 
	CURL* curl = curl_easy_init();
	FILE* filepointer = fopen(outputName.c_str(), "wb"); 
	if (!curl) {return -1; } 
	curl_easy_setopt(curl, CURLOPT_URL, Address.c_str());

	struct curl_slist* headers = NULL; 
	// headers = curl_slist_append(headers, ("User-Agent: " + Agent).c_str()); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, filepointer);

	if (curl_easy_perform(curl)) {return -1; }
	fclose(filepointer); 
	curl_easy_cleanup(curl); 
	curl = NULL; 
	return 0; 
}

int makeDirectory(std::string directoryPath) {
	errno = 0; 
	mkdir(directoryPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO); 
	switch (errno) {
		case 17:
		case 0: 
			return 0; 
		default:
			return -1; 
	}; 
}

struct ListEntry {
	std::string name, address; 
}; 

std::vector<ListEntry> parseProgramList() {
	std::vector<ListEntry> Res; 
	std::string line; 
	std::ifstream InstallList; 
	ListEntry buffer; 
	InstallList.open("programs.csv"); 
	if (InstallList.is_open()) {
		while (std::getline(InstallList, line)) {
			int commapos = line.find(','); 
			buffer.name = line.substr(0, commapos); 
			buffer.address = line.substr(commapos+1); 
			Res.push_back(buffer); 
		}
	}
	return Res; 
}; 

int main() {
	if (makeDirectory("/tmp/Setuper")) {return 1;}
	std::vector<ListEntry> ProgramList = parseProgramList(); 
	FILE *DMG, *Output = NULL; 
	std::string programPath, extractPath; 
	MountType PartType; 
	for (int i = 0; i != ProgramList.size(); i++) {
		PartType = none; 
		std::cout << "current program: " << ProgramList[i].name << "\n"; 
		std::string dmg = "/tmp/Setuper/"+ProgramList[i].name+".dmg"; 
		std::string extract = "/tmp/Setuper/"+ProgramList[i].name+".img"; 
		std::string MountPath = "/tmp/Setuper/"+ProgramList[i].name+"/"; 
		std::string CopyFrom = MountPath+ProgramList[i].name+".app"; 
		std::string CopyTo = "/Applications/"+ProgramList[i].name+".app"; 
		if (makeDirectory("/tmp/Setuper")) {return 1;}
		if (request(ProgramList[i].address, dmg)) {std::cerr << "can't make a request for application "+ProgramList[i].name << "\n"; return 1;}
		DMG = fopen(dmg.c_str(), "rb"); 
		Output = fopen(extract.c_str(), "wb"); 
		if (readDMG(DMG, Output, PartType)) {std::cout << "can't convert the DMG file.\n"; continue;}
		switch (PartType) {
			case hfs: 
				MountHFS((char*)extract.c_str(), (char*)MountPath.c_str(), (char*)(ProgramList[i].name.c_str())); 
				break;
			case apfs: 
				std::cout << "APFS not supported yet\n"; 
				break;
			case none: 
				std::cout << "unknown filesystem, not mounting\n"; 
				break;
		}
	}
	return 0; 
}