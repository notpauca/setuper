#include <iostream>
#include <curl/curl.h>
#include <fstream>
#include <sys/stat.h>
#include <DiskArbitration/DADisk.h>
#include <DiskArbitration/DiskArbitration.h>

const std::string Agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.82 Safari/537.36";

size_t WriteCallback(char *contents, size_t size, size_t nmemb, FILE *filestream)
{
    size_t res = fwrite(contents, size, nmemb, filestream);
    return res; 
}

void request(std::string Address, std::string outputName) {
    curl_global_init(CURL_GLOBAL_ALL); 
    CURL* curl = curl_easy_init();
    FILE* filepointer = fopen(outputName.c_str(), "wb"); 
    if (!curl) {
        std::cerr << "Curl did not initialize."; 
        return; 
    } 
    curl_easy_setopt(curl, CURLOPT_URL, Address.c_str());

    struct curl_slist* headers = NULL; 
    // headers = curl_slist_append(headers, ("User-Agent: " + Agent).c_str()); 
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, filepointer);

    CURLcode res = curl_easy_perform(curl); 
    if (res) {
        std::cerr << curl_easy_strerror(res); 
    }
    fclose(filepointer); 
    curl_easy_cleanup(curl); 
    curl = NULL; 
}

class DMGFile {
    public:
        void mountDMG(std::string DMGName) {
            DMGName = "hdiutil attach "+DMGName; 
            system(DMGName.c_str()); 
        }
        void copyFromDMG (std::string subject) {
            std::filesystem::copy("/Volumes/"+subject+"/"+subject+".app", "/Applications/"+subject+".app", std::filesystem::copy_options::recursive | std::filesystem::copy_options::update_existing);
        }
        void umountDMG(std::string VolumeName) {
            VolumeName = "hdiutil detach /Volumes/"+VolumeName; 
            system(VolumeName.c_str()); 
        }
}; 

int makeDirectory(std::string directoryPath) {
    errno = 0; 
    mkdir(directoryPath.c_str(), 0777); 
    switch errno {
        case 17:
            return 0; 
        case 0: 
            return 0; 
        default:
            return 1; 
    }
}

struct ListEntry {
    std::string name, address, bundleName; 
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
}

int main() {
    if (makeDirectory("/tmp/Setuper")) {return 1;}
    std::vector<ListEntry> ProgramList = parseProgramList(); 
    DMGFile mountingOperations; 
    for (int i = 0; i != ProgramList.size(); i++ ) {
        request(ProgramList[i].address, "/tmp/Setuper/"+ProgramList[i].name+".dmg"); 
        mountingOperations.mountDMG("/tmp/Setuper/"+ProgramList[i].name+".dmg"); 
        mountingOperations.copyFromDMG(ProgramList[i].name); 
        mountingOperations.umountDMG(ProgramList[i].name); 
    }
    return 0; 
}

