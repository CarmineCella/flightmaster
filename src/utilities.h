// utilities.h
// 

#ifndef UTILITIES_H
#define UTILITIES_H 

#include <curl/curl.h>
#include <sys/stat.h>

#include <stdexcept>
#include <memory>
#include <sstream>
#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <fstream>
#include <codecvt>
#include <map>
#include <chrono>
#include <iomanip>
#include <set>

#include <cmath>
#include <cstdio>

#define VERSION 0.5

#define BOLDWHITE   "\033[1m\033[37m"
#define BOLDCYAN    "\033[1m\033[36m"
#define BOLDYELLOW  "\033[1m\033[33m"
#define BOLDBLUE    "\033[1m\033[34m"
#define RED     	"\033[31m" 
#define RESET   	"\033[0m"

struct MemoryStruct {
    char *memory;
    size_t size;
};

size_t write_mem_callback (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char*) realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        /* out of memory! */
        throw std::runtime_error ("not enough memory (realloc returned NULL)");
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

size_t write_data_callback (void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite (ptr, size, nmemb, stream);
    return written;
}

std::string fetch_url_data (const std::string&  url) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = (char*) malloc(1);
    chunk.size = 0; 

    curl_global_init (CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init ();

    curl_easy_setopt (curl_handle, CURLOPT_URL, url.c_str ());
    curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, write_mem_callback);
    curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt (curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl_handle);

    std::stringstream data;
    if (res == CURLE_OK) {
        data << chunk.memory;
    }

    curl_easy_cleanup(curl_handle);
    free (chunk.memory);
    curl_global_cleanup();
    return data.str ();
}

bool download_file (const std::string& url, const std::string& outfilename) {
    CURL *curl_handle = curl_easy_init();
    if (curl_handle) {
        FILE *fp = fopen (outfilename.c_str (),"wb");
        if (fp == 0) return false;

        curl_easy_setopt (curl_handle, CURLOPT_URL, url.c_str ());
        curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, write_data_callback);
        curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, fp);
        CURLcode res = curl_easy_perform (curl_handle);

        curl_easy_cleanup(curl_handle);
        fclose(fp);
        if (res != CURLE_OK) return false;
        else return true;
    }
    return false;
}
std::string trim (const std::string& str, const std::string& newline = "\r\n") {
    const auto strBegin = str.find_first_not_of (newline);
    if (strBegin == std::string::npos)  return ""; // no content
    const auto strEnd = str.find_last_not_of (newline);
    const auto strRange = strEnd - strBegin + 1;
    return str.substr (strBegin, strRange);
}

std::string html2text (std::string htmlTxt) {
    std::regex stripFormatting ("<[^>]*(>|$)"); //match any character between '<' and '>', even when end tag is missing

    std::string s1 = std::regex_replace (htmlTxt, stripFormatting, "");
    std::string s2 = trim (s1);
    std::string s3 = std::regex_replace (s2, std::regex("\\&nbsp;"), ",");
    return s3;
}

std::string quote (const std::string input) {
    return (std::string) "\"" + input + (std::string) "\"";
}
 
std::string unquote (const std::string input) {
    std::string s = input;
    s.erase (
        remove (s.begin (), s.end (), '\"'), s.end ());
    return s;
}

typedef std::vector<std::pair<std::string, std::vector<std::string>>> CSV_data;
// typedef std::map<std::string, std::vector<std::string>> CSV_data;

CSV_data read_csv (std::istream& input){
    // read a CSV file into a vector of <string, vector<string>> pairs where
    // each pair represents <column name, column values>    
    CSV_data result;
    std::string line, colname;
    std::string val;

    // extract the first line in the file
    std::getline (input, line);
    std::stringstream ss (line);

    // extract each column name
    while (std::getline (ss, colname, ',')) {
        result.push_back({colname, std::vector<std::string> {}});
    }
   
   if (result.size () < 1) return result;

   // read data, line by line
    while (std::getline (input, line))  {
        std::stringstream ss (line);
        int colIdx = 0;
        while (std::getline (ss, val, ',')){
            result.at (colIdx).second.push_back(val);
            // if (ss.peek () == ',') ss.ignore ();
            colIdx++;
            if (colIdx >= result.size ()) break;
        }
    }

    return result;
}

std::vector<std::string>& get_csv_column (const std::string& colname, CSV_data& matrix) {
    for (unsigned int i = 0; i < matrix.size (); ++i) {
        std::cout << "--- " << colname << ", " << matrix.at (i).first << " " << quote(colname) << std::endl;
        if (matrix.at (i).first == quote (colname)) {
            return matrix.at (i).second;
        }
    }
    throw std::runtime_error ("invalid CSV column requested");
}

std::vector<int> get_csv_rows_by_key (const std::string& colname, const std::string& key, CSV_data& matrix) {
    std::vector<int> idx;
    std::vector<std::string> col = get_csv_column (colname, matrix);
    for (unsigned i = 0; i < col.size (); ++i) {
        if (col.at (i) == key) {
            idx.push_back (i);
        }
    }
    return idx;
}

std::string get_file_date (const std::string& filename) {
    struct stat t_stat;
    stat (filename.c_str (), &t_stat);
    struct tm* timeinfo = localtime (&t_stat.st_ctime); // or gmtime() depending on what you want
    // std::stringstream date;
    // date << timeinfo->tm_mon << "/" << timeinfo->tm_mday << "/" << 1900 + timeinfo->tm_year;
    return asctime (timeinfo); //date.str ();
}

std::string string_time (int time_in_sec) {
    int minutes = (int) time_in_sec / 60.;
    int seconds = time_in_sec  % 60;
    int hours = (int) minutes / 60;
    minutes = minutes % 60;
    std::stringstream ttime;
    if (hours) ttime << std::setw (2) << std::setfill ('0') << hours << ":";
    ttime << std::setw (2) << std::setfill ('0') << minutes << ":"  
        << std::setw (2) << std::setfill ('0') << seconds;
    return ttime.str ();
}     

#endif	// UTILITIES_H 

// EOF
