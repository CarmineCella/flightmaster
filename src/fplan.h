// fplan.h
// 

#ifndef FPLAN_H
#define FPLAN_H 

#include <curl/curl.h>

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

#include <cmath>

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

std::string fetch_url_data (const std::string url) {
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
    if (res != CURLE_OK) {
        throw std::runtime_error ("unable to fetch online data");
    } else {
        data << chunk.memory;
    }

    curl_easy_cleanup(curl_handle);
    free (chunk.memory);
    curl_global_cleanup();
    return data.str ();
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

struct Fix {
    // input
    std::string name;
    int true_course;
    double distance;
    std::string wind_station;

    // output
    int true_heading;
    int magnetic_course;
    int magnetic_heading;
    std::string ETE;

    // fetch
    std::vector<std::string> navaids;
    std::string winds_aloft;
};

struct Parameters {
    // input
    std::string departure;
    double cruise_IAS;
    double fuel_per_hour;
    double magnetic_variation;
    std::vector<Fix> fixes;
    std::vector<std::string> custom_frequencies;

    // output
    double total_miles;
    double total_time;

    // fetch
    std::vector<std::string> airports;
    std::vector<std::string> metars;
    std::vector<std::string> tafs;
};

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
            //if (ss.peek () == ',') ss.ignore ();
            colIdx++;
            if (colIdx >= result.size ()) break;
        }
    }

    return result;
}

std::vector<std::string>& get_csv_column (const std::string& colname, CSV_data& matrix) {
    for (unsigned int i = 0; i < matrix.size (); ++i) {
        if (matrix.at (i).first == quote (colname)) return matrix.at (i).second;
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
    if (idx.size () == 0) throw std::runtime_error ("CSV key not found");
    return idx;
}

std::vector<std::string> get_metar_taf (const std::string station, const std::string& source) {
    std::stringstream query;
    query << "https://www.aviationweather.gov/adds/dataserver_current/httpparam?&requestType=retrieve&format=csv&hoursBeforeNow=1&mostRecent=true&stationString=";
    query << station << "&dataSource=" << source;
    std::string info = fetch_url_data (query.str ());

    std::stringstream info_stream;
    info_stream << info;

    std::string dummy; // remove headers (5 lines)
    for (unsigned i = 0; i < 5; ++i) getline (info_stream, dummy);

    CSV_data result = read_csv (info_stream);
    
    std::vector<std::string> info_vector;
    for (int i = 0; i < result.size (); ++i) {
        if (result.at (i).first == "raw_text") {
            for (unsigned k = 0; k < result.at (i).second.size (); ++k) {
                info_vector.push_back (result.at (i).second.at (k));
            }   
        }
    }

    return info_vector;
}

std::string get_winds_aloft (const std::string station) {
    std::stringstream query;
    query << "https://www.aviationweather.gov/windtemp/data?region=all&layout=off";
    std::string info = fetch_url_data (query.str ());
    
    std::stringstream info_stream;
    info_stream << info;
    std::vector<std::string> info_vector;

    std::string line;
    std::string result;
    while (getline (info_stream, line)) {
        if (line.find (station) != std::string::npos) {
            result = line;
            break;
        }
    }
    return result;
}

void get_wind_info (const std::string& winds_aloft, double altitude, int& direction, int& speed) {
    //FT  3000    6000    9000   12000   18000   24000  30000  34000  39000
    //SFO 9900 0706+20 0909+15 1114+09 0618-07 0424-20 062636 062347 051857

    int ipos = 0;
    if (altitude < 6000) ipos = 4; 
    else if (altitude >= 6000 && altitude < 9000) ipos = 9; 
    else if (altitude >= 9000 && altitude < 12000) ipos = 17; 
    else if (altitude >= 12000 && altitude < 18000) ipos = 25;
    else if (altitude >= 18000 && altitude < 24000) ipos = 33; 
    else if (altitude >= 24000 && altitude < 30000) ipos = 41; 
    else throw std::runtime_error ("unsupported alitude for wind correction");
    
    direction = atol (winds_aloft.substr (ipos, 2).c_str ());
    speed = atol (winds_aloft.substr (ipos + 2, 2).c_str ());
    if (direction == 99) return;
    if (direction >= 51) {
        direction -= 50;
        speed += 100;
    }
    direction *= 10;
}

int compute_wind_correction (int wind_direction, int wind_speed, int IAS, int intended_course) {
    double indended_course_rad = M_PI * (double) intended_course / 180.; // keep all in radians internally
    double wind_direction_rad = M_PI * (double) wind_direction  / 180. + M_PI;
    
    if (wind_direction_rad > M_PI) wind_direction_rad = wind_direction_rad - 2. * M_PI;
  
    double WT_angle = (indended_course_rad - wind_direction_rad);
    if (WT_angle > M_PI)  WT_angle = WT_angle - 2. * M_PI;
    if (WT_angle < -M_PI) WT_angle = WT_angle + 2. * M_PI;
    
    double sin_WCA = (double) wind_speed * sin (WT_angle) / (double) IAS;
    if (fabs (sin_WCA) < 1.) {
        double WCA = asin (sin_WCA);
        double heading_rad = indended_course_rad + WCA;
        while (heading_rad > 2. * M_PI) heading_rad = heading_rad - 2. * M_PI;
        while (heading_rad < 0.) heading_rad = heading_rad + 2. * M_PI;
        return (int) (180. * heading_rad / M_PI); // convert in degrees
    }

    throw std::runtime_error ("invalid wind parameters");
}

std::vector<std::string> get_navaids (const std::string& station) {
    std::stringstream query;
    query << "https://www.airnav.com/airport/" << station;

    std::string info = fetch_url_data (query.str ());
    std::string t = html2text (info);

    std::stringstream t_stream;
    t_stream << t;
    std::string line;
    std::vector<std::string> info_vector;
    while (getline (t_stream, line)) {
        if (line.find ("VOR") != std::string::npos && line.find (",") == std::string::npos) {
            int pos = line.size ();
            for (unsigned i = 3; i < line.size (); ++i) {
                if (isupper (line.at (i))) {
                    pos = i;
                    break;
                }
            }
            std::stringstream fline;
            fline << line.substr (0, 3) << " " << line.substr (3, pos - 3) << " " << line.substr (pos, line.size () - pos);
            info_vector.push_back (fline.str ());
        }
    }
    return info_vector;
}

void load_dbs (CSV_data& airports, CSV_data& frequencies, CSV_data& runways) {
    std::string airports_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airports.csv";
    std::string frequencies_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airport-frequencies.csv";
    std::string runways_db = (std::string) getenv("HOME") + (std::string) "/.fplan/runways.csv";

	std::ifstream airports_in (airports_db.c_str());
	if (!airports_in.good ()) throw std::runtime_error ("cannot load airports database");
    std::ifstream frequencies_in (frequencies_db.c_str());
	if (!frequencies_in.good ()) throw std::runtime_error ("cannot load frequencies database");
    std::ifstream runways_in (runways_db.c_str());
	if (!runways_in.good ()) throw std::runtime_error ("cannot load runways database");

    airports = read_csv (airports_in);
    frequencies = read_csv (frequencies_in);
    runways = read_csv (runways_in);
}

std::vector<std::string> get_airport_info (const std::string& station, 
    CSV_data& airports,
    CSV_data& frequencies,
    CSV_data& runways) {

    std::vector<std::string> result;
    std::stringstream r;
    std::vector<int> idx = get_csv_rows_by_key ("ident", quote (station), airports);
    r << unquote (get_csv_column ("ident", airports).at (idx.at (0))) 
        << " (" << unquote (get_csv_column ("name", airports).at (idx.at (0))) 
        << ") - elevation (ft): " << get_csv_column ("elevation_ft", airports).at (idx.at (0));
    std::cout << r.str () << std::endl;
    result.push_back (r.str ());
    
    idx.clear ();
    idx = get_csv_rows_by_key ("airport_ident", quote (station), runways);
    for (unsigned i = 0; i < idx.size (); ++i) {
        std::stringstream r; // local
        r << "RWY " <<  unquote (get_csv_column ("le_ident", runways).at (idx.at (i))) << "/" 
            << unquote (get_csv_column ("he_ident", runways).at (idx.at (i))) << " (" 
            <<  get_csv_column ("length_ft", runways).at (idx.at (i)) << " x " 
            << get_csv_column ("width_ft", runways).at (idx.at (i)) << " ft)";
        std::cout << r.str () << std::endl;
        result.push_back (r.str ());
    }

    idx.clear ();
    idx = get_csv_rows_by_key ("airport_ident", quote (station), frequencies);
    for (unsigned i = 0; i < idx.size (); ++i) {
        std::stringstream r; // local
        r << unquote (get_csv_column ("description", frequencies).at (idx.at (i))) << " "
            << get_csv_column ("frequency_mhz", frequencies).at (idx.at (i)) << " MHz";
        std::cout << r.str () << std::endl;
        result.push_back (r.str ());
    }
    return result;
}


#endif	// FPLAN_H 

// EOF
