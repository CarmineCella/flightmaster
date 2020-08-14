// fplan.h
// 

#ifndef FPLAN_H
#define FPLAN_H 

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

// ---------------------------------------------
class FlightLog;

struct Fix {
    Fix () {
        name = "";
        true_course = 0;
        altitude = 0;
        distance = 0;
        wind_station = "";

        winds_aloft = "";

        true_heading = 0;
        magnetic_course = 0;
        magnetic_heading = 0;
        ETE = 0;
        fuel = 0;
    }

    // input
    std::string name;
    int true_course;
    int altitude;
    double distance;
    std::string wind_station;
    std::vector<std::string> navaids; // can be fetched

    // fetch
    std::string winds_aloft;

    // output
    int true_heading;
    int magnetic_course;
    int magnetic_heading;
    int ETE; // in sec.
    double fuel;

};

struct Airport {
    Airport () {
        info = "";
        longit = 0;
        lat = 0;
    }
    std::string info;
    std::vector<std::string> runways;
    std::vector<std::string> frequencies;
    double longit;
    double lat;
};

struct Parameters {
    Parameters () {
        departure = "";
        alternate = "";
        cruise_IAS = 0;
        fuel_per_hour = 0;
        taxi_fuel = 0;
        reserve_time = 0;
        magnetic_variation = 0;
        total_miles = 0;
        total_time = 0;
        WCAmax = 0;
    }

    // input
    std::string departure;
    std::string alternate;
    double cruise_IAS;
    double fuel_per_hour;
    double taxi_fuel;
    double reserve_time;
    int magnetic_variation;
    std::vector<Fix> fixes;
    std::vector<std::string> custom;
    CSV_data airports_db, frequencies_db, runways_db, navaids_db;

    // output
    double total_miles;
    int total_time; // in sec.
    double total_fuel;
    int WCAmax;
};

// ----------------------------------------------
void update_dbs (std::ostream& out) {
    mkdir (((std::string) getenv("HOME") + (std::string) "/.fplan/").c_str (), 0777);
    std::string airports_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airports.csv";
    std::string frequencies_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airport-frequencies.csv";
    std::string runways_db = (std::string) getenv("HOME") + (std::string) "/.fplan/runways.csv";
    std::string navaids_db = (std::string) getenv("HOME") + (std::string) "/.fplan/navaids.csv";

    out << "downloading airports......"; out.flush ();
	if (!download_file ("https://ourairports.com/data/airports.csv", airports_db)) {
        out << RED << "failed" << std::endl;
    } else {
        out << "latest " << get_file_date (airports_db);
    }
    out << "downloading frequencies..."; out.flush ();
    if (!download_file ("https://ourairports.com/data/airport-frequencies.csv", frequencies_db)) {
        out << RED << "failed" << std::endl;
    } else {
        out << "latest " << get_file_date (frequencies_db);
    }
    out << "downloading runways......."; out.flush ();
    if (!download_file ("https://ourairports.com/data/runways.csv", runways_db)) {
        out << RED << "failed" << std::endl;
    } else {
        out << "latest " << get_file_date (runways_db);
    }
    out << "downloading navaids......."; out.flush ();    
    if (!download_file ("https://ourairports.com/data/navaids.csv", navaids_db)) {
        out << RED << "failed" << std::endl;
    } else {
        out << "latest " << get_file_date (navaids_db);
    }
    out << std::endl << std::endl;

    out << BOLDCYAN << "warning: the US waypoints database (FIX.txt) needs to be manually updated\n"
        << "from https://www.faa.gov/air_traffic/flight_info/aeronav/aero_data/" << RESET << std::endl << std::endl;
}
void load_dbs (std::ostream& out, 
    CSV_data& airports, CSV_data& frequencies, CSV_data& runways, CSV_data& navaids) {
    std::string airports_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airports.csv";
    std::string frequencies_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airport-frequencies.csv";
    std::string runways_db = (std::string) getenv("HOME") + (std::string) "/.fplan/runways.csv";
    std::string navaids_db = (std::string) getenv("HOME") + (std::string) "/.fplan/navaids.csv";

    out <<  RESET << "airports......"; out.flush ();
    std::ifstream airports_in (airports_db.c_str());
	if (!airports_in.good ()) {
        out << RED << "failed" << std::endl;
    } else {
        out  << "latest " << get_file_date (airports_db);
    }
    out <<  RESET << "frequencies..."; out.flush ();
    std::ifstream frequencies_in (frequencies_db.c_str());
    if (!frequencies_in.good ()) {
        out << RED << "failed" << std::endl;
    } else {
        out  << "latest " << get_file_date (frequencies_db);
    }
    out <<  RESET << "runways......."; out.flush ();
    std::ifstream runways_in (runways_db.c_str());
    if (!runways_in.good ()) {
        out << RED << "failed" << std::endl;
    } else {
        out << "latest " << get_file_date (runways_db);
    }
    out <<  RESET << "navaids......."; out.flush ();    
    std::ifstream navaids_in (navaids_db.c_str());
    if (!navaids_in.good ()) {
        out << RED << "failed" << std::endl;
    } else {
        out  << "latest " << get_file_date (navaids_db);
    }
    out <<  RESET << std::endl;

    airports = read_csv (airports_in);
    frequencies = read_csv (frequencies_in);
    runways = read_csv (runways_in);
    navaids = read_csv (navaids_in);
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

bool get_wind_info (const std::string& winds_aloft, double altitude, int& direction, int& speed) {
    //FT  3000    6000    9000   12000   18000   24000  30000  34000  39000
    //SFO 9900 0706+20 0909+15 1114+09 0618-07 0424-20 062636 062347 051857

    if (winds_aloft.size () == 0) return false;
    
    int ipos = 0;
    if (altitude < 6000) ipos = 4; 
    else if (altitude >= 6000 && altitude < 9000) ipos = 9; 
    else if (altitude >= 9000 && altitude < 12000) ipos = 17; 
    else if (altitude >= 12000 && altitude < 18000) ipos = 25;
    else if (altitude >= 18000 && altitude < 24000) ipos = 33; 
    else if (altitude >= 24000 && altitude < 30000) ipos = 41; 
    else return false;
    
    direction = atol (winds_aloft.substr (ipos, 2).c_str ());
    speed = atol (winds_aloft.substr (ipos + 2, 2).c_str ());
    if (direction == 99) return true;
    if (direction >= 51) {
        direction -= 50;
        speed += 100;
    }
    direction *= 10;
    return true;
}

#define d2r (M_PI / 180.0)
#define r2d (180.0 / M_PI)

int compute_wind_correction (int wind_direction, int wind_speed, int IAS, int intended_course, double& ground_speed) {
    double indended_course_rad = (double) intended_course * d2r; // keep all in radians internally
    double wind_direction_rad = (double) wind_direction * d2r  + M_PI; // from -> to
    
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
        ground_speed = IAS * cos (WCA) + wind_speed * cos (WT_angle);
        return (int) (heading_rad * r2d); // convert in degrees
    }
    throw std::runtime_error ("invalid wind parameters");
}

void compute_xwind (int heading, int wind_dir, double wind_speed, double& xwind_speed, double& tailwind_speed) {
    double  WAA = (double) (wind_dir - heading);
    xwind_speed = wind_speed * sin ((double) WAA * d2r);
    tailwind_speed = wind_speed * cos ((double) WAA * d2r);
}

double haversine_mi (double lat1, double long1, double lat2, double long2) {
    double dlong = (long2 - long1) * d2r;
    double dlat = (lat2 - lat1) * d2r;
    double a = pow (sin(dlat/2.0), 2) + cos(lat1*d2r) * cos(lat2*d2r) * pow(sin(dlong/2.0), 2);
    double c = 2 * atan2 (sqrt(a), sqrt(1-a));
    double d = 3956 * c;
    return d;
}

int  bearing_from_gps (double lat1, double long1, double lat2, double long2) {
    double dlong = (long2 - long1) * d2r;
    double X = cos (lat2 * d2r) * sin (dlong);
    double Y = cos (lat1 * d2r) * sin (lat2  * d2r) - sin (lat1 * d2r) * cos(lat2 * d2r) * cos (dlong);
    double b = atan2 (X, Y);
    return ((int) (b * r2d) + 360) % 360;
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
            fline << line.substr (0, 3) << " " << line.substr (3, pos - 3);
            info_vector.push_back (fline.str ());
        }
    }
    return info_vector;
}

Airport get_airport_info (const std::string& station, 
    CSV_data& airports,
    CSV_data& frequencies,
    CSV_data& runways) {

    std::vector<Airport> result;
    Airport apt;

    std::stringstream r;
    std::vector<int> idx = get_csv_rows_by_key ("ident", quote (station), airports);
    if (idx.size () == 0) return apt;
    
    r << unquote (get_csv_column ("ident", airports).at (idx.at (0))) 
        << " (" << unquote (get_csv_column ("name", airports).at (idx.at (0))) 
        << ") - elev. " << get_csv_column ("elevation_ft", airports).at (idx.at (0)) << " ft";
    apt.info = r.str ();
    
    apt.lat =  atof (get_csv_column ("latitude_deg", airports).at (idx.at (0)).c_str ());
    apt.longit = atof (get_csv_column ("longitude_deg", airports).at (idx.at (0)).c_str ());

    idx.clear ();
    idx = get_csv_rows_by_key ("airport_ident", quote (station), runways);
    for (unsigned i = 0; i < idx.size (); ++i) {
        std::stringstream r; // local
        r << "RWY " <<  unquote (get_csv_column ("le_ident", runways).at (idx.at (i))) << "/" 
            << unquote (get_csv_column ("he_ident", runways).at (idx.at (i))) << " (" 
            <<  get_csv_column ("length_ft", runways).at (idx.at (i)) << " x " 
            << get_csv_column ("width_ft", runways).at (idx.at (i)) << " ft)";
        apt.runways.push_back (r.str ());
    }

    idx.clear ();
    idx = get_csv_rows_by_key ("airport_ident", quote (station), frequencies);
    for (unsigned i = 0; i < idx.size (); ++i) {
        std::stringstream r; // local
        r << unquote (get_csv_column ("description", frequencies).at (idx.at (i))) << " "
            << get_csv_column ("frequency_mhz", frequencies).at (idx.at (i)) << " MHz";
        apt.frequencies.push_back (r.str ());
    }
    return apt;
}

std::vector<std::string> get_navaid_info (const std::string& station, 
    CSV_data& navaids) {
    std::vector<std::string> result;
    std::vector<int> idx = get_csv_rows_by_key ("ident", quote (station), navaids);
    if (idx.size () == 0) return result;        

    for (int i = 0; i < idx.size (); ++i) {
        std::stringstream r;
        r << unquote (get_csv_column ("ident", navaids).at (idx.at (i))) 
            << " (" << unquote (get_csv_column ("name", navaids).at (idx.at (i))) 
            << ") - " << unquote (get_csv_column ("type", navaids).at (idx.at (i))) << " "
            << get_csv_column ("frequency_khz", navaids).at (idx.at (i)) << " KHz";
        result.push_back (r.str());
    }
    return result;
}

void set_parameter (std::deque<std::string>& tokens, Parameters& p, std::ostream& out) {
    if (tokens[0] == "departure") {
        p.departure = tokens[1];
        Airport a = get_airport_info (p.departure, p.airports_db, p.frequencies_db, p.runways_db);
        if (a.info.size () == 0) {
            throw std::runtime_error ("departure is not an airport");
        }
    } else if (tokens[0] == "alternate") {
        p.alternate = tokens[1];
        Airport a = get_airport_info (p.alternate, p.airports_db, p.frequencies_db, p.runways_db);
        if (a.info.size () == 0) {
            throw std::runtime_error ("alternate is not an airport");
        }
    }  else if (tokens[0] == "cruise_speed") {
    	p.cruise_IAS  = atof (tokens[1].c_str ());
        if (p.cruise_IAS <= 60) {
             throw std::runtime_error ("cruise speed is smaller than 60");
        }
    } else if (tokens[0] == "fuel_per_hour") {
    	p.fuel_per_hour = atof (tokens[1].c_str ());
        if (p.fuel_per_hour <= 0) {
             throw std::runtime_error ("invalid fuel per hour");
        }        
    } else if (tokens[0] == "taxi_fuel") {
    	p.taxi_fuel = atof (tokens[1].c_str ());
        if (p.fuel_per_hour <= 0) {
             throw std::runtime_error ("invalid taxi fuel");
        }        
    } else if (tokens[0] == "reserve_time") {
    	p.reserve_time = atof (tokens[1].c_str ()) * 60;
        if (p.reserve_time <= 0) {
             throw std::runtime_error ("invalid reserve time");
        }        
    } else if (tokens[0] == "magnetic_variation") {
        if (tokens.size () != 3) {
            throw std::runtime_error ("invalid syntax for magnetic variation");
        }
        int deg = atol (tokens[1].c_str ());
        if (tokens[2] == "W") p.magnetic_variation = deg;
        else if (tokens[2] == "E") p.magnetic_variation = -deg;
        else {
            throw std::runtime_error ("magnetic variation must but E or W");
        }
        if (abs (p.magnetic_variation) > 180) {
             throw std::runtime_error ("abnormal magnetic variation");
        }        
    } else if (tokens[0] == "custom") {
        std::stringstream t;
    	for (unsigned i = 1; i < tokens.size (); ++i) {
    		t << tokens[i] << " ";
    	}
        p.custom.push_back (t.str ());
    } else if (tokens[0] == "fix") {
        if (tokens.size () < 7) {
            throw std::runtime_error ("invalid syntax for fix");
        }
        Fix f;
        f.name = tokens[1];
        if (tokens[2] == "auto") {
            f.true_course = 999;
        } else {
            f.true_course = atol (tokens[2].c_str ());
            if (f.true_course < 0 || f.true_course > 359) {
                throw std::runtime_error ("invalid true course");
            }
        }
        f.altitude = atol (tokens[3].c_str ());
        if (f.altitude < 0 | f.altitude > 30000) {
            throw std::runtime_error ("invalid altitude");
        }        
        if (tokens[4] == "auto") {
            f.distance = -1;
        } else {
            f.distance = atof (tokens[4].c_str ());
            if (f.distance <= 0 || f.distance > 999) {
                throw std::runtime_error ("invalid distance");
            }        
        }
        f.wind_station = tokens[5];
        if (f.wind_station != "none") {
            f.winds_aloft = get_winds_aloft (f.wind_station);
            if (f.winds_aloft.size () == 0) {
                out << BOLDCYAN << "warning: cannot fetch winds for " << f.name << RESET << std::endl;
            }
        } 
        
        // navaids
        for (unsigned i = 6; i < tokens.size (); ++i) {
            std::string navaid = tokens.at (i);
            if (navaid == "none") {
                break;
            } else if (navaid.size () <= 4) {
                f.navaids = get_navaids (navaid);
                if (f.navaids.size () > 2) f.navaids.resize (2); // cut to two
                if (f.navaids.size () == 0) {
                    out << BOLDCYAN << "warning: cannot fetch navaids for " << f.name << RESET << std::endl;
                }
                break;
            } else {
                std::stringstream ss;
                ss << navaid;
                std::string val;
                std::vector<std::string> c;
                while (std::getline (ss, val, ',')){
                    c.push_back(val);
                }                
                if (c.size () != 3) {
                    throw std::runtime_error ("invalid syntax for navaid");
                }

                std::vector<std::string> info = get_navaid_info (c.at (0), p.navaids_db);
                if (info.size () == 0) throw std::runtime_error ("invalid navaid identifier");
                int deg = atol (c.at (1).c_str ());
                if (deg < 0 || deg > 359) throw std::runtime_error ("invalid radial");
                double dist = atof (c.at (2).c_str ());
                if (dist <= 0)throw std::runtime_error ("invalid navaid distance");
                std::stringstream t;
                t << c.at (0) << "  r" << deg << "/" << dist;
                f.navaids.push_back (t.str ());
            }
        }
        p.fixes.push_back (f);
    } else {
        throw std::runtime_error ("invalid command in configuration file");
    }
}
// ---------------------------------------------------------
void read_config (const char* config_file, Parameters& p, std::ostream& out) {
    std::ifstream config (config_file);
    if (!config.good ()) {
        throw std::runtime_error ("cannot open configuration file");
    }

    int line = 0;
    while (!config.eof ()) {
        std::string inp;
        std::string opcode;

        ++line;
        std::getline (config, inp, '\n');

        inp = trim (inp);
        if (inp.size () == 0) continue;

        std::istringstream istr (inp, std::ios_base::out);

        std::deque <std::string> tokens;
        while (!istr.eof ()) {
            istr >> opcode;
            tokens.push_back (opcode);
        }

        if (tokens.size () < 2) {
            std::stringstream err;
            err << "invalid syntax at line " << line;
            throw std::runtime_error (err.str ());
        }
        if (tokens[0].size () && tokens[0][0] == ';') continue;
        set_parameter (tokens, p, out);
    }

    if (p.departure == "" || p.alternate == "" || p.cruise_IAS == 0 || p.fuel_per_hour == 0 || p.fixes.size () == 0
        || p.taxi_fuel == 0 || p.reserve_time == 0) {
        throw std::runtime_error ("missing parameters in configuration file");
    }
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

std::string compile_flight (Parameters& p, std::ostream& out) {
    std::stringstream flight_log;
    p.WCAmax = (int) (180. * asin (10. / (double) p.cruise_IAS) / M_PI);
    auto start = std::chrono::system_clock::now();
    std::time_t curr_time = std::chrono::system_clock::to_time_t(start);
    flight_log << "compiled on " << std::ctime (&curr_time) << std::endl;

    flight_log << "**DEPARTURE: " << p.departure << "**" << std::endl << std::endl;
    flight_log << "Date _________ Tach _________ Block _________" << std::endl << std::endl;
    flight_log << "Info _________ W/V  _________ QNH   _________" << std::endl << std::endl;
    flight_log << "Taxi _________ RWY  _________ T/O   _________" << std::endl << std::endl << std::endl;

    flight_log << "**FLIGHT LOG**" << std::endl << std::endl;
    flight_log << "|      FIX |  MC |  MH |     ALT |    NM |       NAVAIDS |      ETE |" << std::endl;
    flight_log << "|----------|-----|-----|---------|-------|---------------|----------|" << std::endl;

    Airport apt = get_airport_info (p.departure, p.airports_db, p.frequencies_db, p.runways_db);
    double prev_lat = apt.lat;
    double prev_long = apt.longit;

    int prev_altitude = 0;
    int prev_fuel_check = 0;
    std::set<std::string> navaids_list;
    std::set<std::string> airports_list;
    std::set<std::string> winds_list;
    for (unsigned i = 0; i < p.fixes.size (); ++i) {
        Fix& f = p.fixes.at (i);
        out << "processing...." << BOLDWHITE << f.name << RESET << std::endl; out.flush ();

        Airport apt = get_airport_info (f.name, p.airports_db, p.frequencies_db, p.runways_db);
        if (apt.info.size ()) airports_list.insert (f.name);
        if (apt.info.size () == 0  && (i == p.fixes.size () - 1  || f.true_course == 999 || f.distance == -1)) {
            throw std::runtime_error ("fix must be an airport if it is the last or if course/distance are set to auto");
        }

        if (f.true_course == 999) {
            f.true_course = bearing_from_gps (prev_lat, prev_long, apt.lat, apt.longit);
        }
        if (f.distance == -1) {
            f.distance = haversine_mi  (prev_lat, prev_long, apt.lat, apt.longit);
        }

        if (apt.info.size ()) {
            double delta_dist = fabs (haversine_mi (prev_lat, prev_long, apt.lat, apt.longit) - f.distance);
            if (delta_dist > 10) {
                std::cout << BOLDCYAN << "warning: specified distance differs from calulated (" 
                    << haversine_mi (prev_lat, prev_long, apt.lat, apt.longit) << " vs " << f.distance << ")" << RESET << std::endl;            
            }
            int tc = bearing_from_gps (prev_lat, prev_long, apt.lat, apt.longit);
            double delta_bearing = fabs (tc - f.true_course);
            if (delta_bearing > 10) {
                std::cout << BOLDCYAN << "warning: true course differs from calulated (" 
                   <<  tc << " vs " << f.magnetic_course << ")" << RESET << std::endl;            
            }
        }

        f.magnetic_course = (f.true_course + p.magnetic_variation + 360) % 360;
        double ground_speed = 0;
        int wdir = 0, wspeed = 0;
        if (get_wind_info (f.winds_aloft, f.altitude, wdir, wspeed)) {
            f.true_heading = compute_wind_correction (wdir, wspeed, p.cruise_IAS, f.true_course, ground_speed);
            f.magnetic_heading = (f.true_heading + p.magnetic_variation + 360) % 360;
        } else f.magnetic_heading = 999;

        double true_speed = ground_speed ? ground_speed : p.cruise_IAS;
        f.ETE = (int) (f.distance / true_speed * 3600.); // in sec.
        f.fuel = f.ETE * p.fuel_per_hour / 3600.;

        p.total_time += f.ETE;
        p.total_miles += f.distance;
        p.total_fuel += f.fuel;

        flight_log << "|" << std::setw (9) << f.name.substr (0, 8) << " ";
        flight_log << "| " << std::setw (3) << std::setfill ('0') << f.magnetic_course 
            << std::setfill (' ') << " ";
        if (f.magnetic_heading == 999) {
                flight_log << "|     "; // no wind correction
        } else flight_log << "| " << std::setfill ('0') << std::setw (3) << f.magnetic_heading 
            << std::setfill (' ') << " "; 
        flight_log << "|" << std::setw (6) << f.altitude << " ";
        if (prev_altitude < f.altitude) flight_log << "U ";
        else if (prev_altitude > f.altitude) flight_log << "D ";
        else flight_log << "_ ";
        flight_log << "|" << std::setw (6) << std::fixed <<  std::setprecision (1) <<  f.distance << " ";
        if (f.navaids.size () == 0) flight_log  << "|               ";
        else flight_log << "|" << std::setw (14) << f.navaids.at (0) << " ";
        flight_log << "| " << std::setw (8) << string_time (f.ETE);
        if (ground_speed != 0) flight_log << "*|" << std::endl;
        else flight_log << " |" << std::endl;
        prev_altitude = f.altitude;         
        for (unsigned i = 1; i < f.navaids.size (); ++i) {
            flight_log << "|          |     |     |         |       ";
            flight_log << "|" << std::setw (14) << f.navaids.at (i) << " ";
            flight_log << "|          |" << std::endl;
        }
        if (p.total_time - prev_fuel_check > (30 * 60)) {
            flight_log << "|----------|-----|-----|---------|-------|---------------|----------|" << std::endl;
            flight_log << "|                                TANK                               |" << std::endl;
            flight_log << "|----------|-----|-----|---------|-------|---------------|----------|" << std::endl;
            prev_fuel_check = p.total_time;
        } else {
            flight_log << "|----------|-----|-----|---------|-------|---------------|----------|" << std::endl;
        }

        winds_list.insert (f.wind_station);
        for (unsigned i = 0; i < f.navaids.size (); ++i) {
            std::string station = f.navaids.at (i).substr (0, 3);
            navaids_list.insert (station);
        }
            
        prev_lat = apt.lat;
        prev_long = apt.longit;            

    }

    flight_log << "|   TOTALS |     |     |         |";
    flight_log << std::setw (6) << std::fixed <<  std::setprecision (1) << p.total_miles << " ";
    flight_log << "|               | " << std::setw (8) << string_time (p.total_time) << " |" 
        << std::endl << std::endl;

    Airport alt = get_airport_info (p.alternate, p.airports_db, p.frequencies_db, p.runways_db);
    int alt_bearing =  (bearing_from_gps (prev_lat, prev_long, alt.lat, alt.longit) + 360) % 360;
    double alt_dist = haversine_mi (prev_lat, prev_long, alt.lat, alt.longit);
    int alt_ETA = (int) (alt_dist / p.cruise_IAS  * 3600.); // in sec.

    if (p.alternate == p.fixes.at (p.fixes.size () - 1).name) {
        throw std::runtime_error ("alternate cannot be the same as arrival");
    }

    std::vector<std::string> alt_aids = get_navaids (p.alternate);
    flight_log << "ALTERNATE " << p.alternate << ", MC " << alt_bearing 
        << ", ETA " << string_time (alt_ETA);
    airports_list.insert (p.alternate);
    if (alt_aids.size ()) {
        flight_log << ", " << alt_aids.at (0);
        flight_log << std::endl << std::endl << std::endl;
        std::string alt_navaid_station = alt_aids.at (0).substr (0, 3);
        navaids_list.insert (alt_navaid_station);
    }

    flight_log << "**ARRIVAL: " << p.fixes.at (p.fixes.size () - 1).name  << "**" << std::endl << std::endl;

    flight_log << "Info _________ W/V   _________ QNH  _________" << std::endl << std::endl;
    flight_log << "RWY  _________ Taxi  _________ Land _________" << std::endl << std::endl;
    flight_log << "Tach _________ Block _________" << std::endl << std::endl << std::endl;

    flight_log << "**FUEL**" << std::endl << std::endl;

    flight_log << std::left << std::setw (10) << "Legs" << std::right << std::setw (10) << p.total_fuel << " lt/gal" << std::endl;
    flight_log <<  std::left << std::setw (10) << "Taxi" << std::right << std::setw (10) << p.taxi_fuel << " lt/gal" << std::endl;
    double alt_fuel = (double) alt_ETA  * p.fuel_per_hour / 3600.;
    flight_log <<  std::left << std::setw (10) << "Alternate" << std::right << std::setw (10) << alt_fuel << " lt/gal" << std::endl;
    double reserve_fuel = (double) p.reserve_time * p.fuel_per_hour / 3600.;
    flight_log <<  std::left << std::setw (10) << "Reserve" << std::right << std::setw (10) << reserve_fuel << " lt/gal" << std::endl;
    flight_log <<  std::left << std::setw (10) << "Total" << std::right <<  std::setw (10) 
        << p.total_fuel + p.taxi_fuel + alt_fuel + reserve_fuel << " lt/gal" << std::endl << std::endl;;

    std::stringstream metars_coll;
    std::stringstream tafs_coll;
    if (airports_list.size ()) {
        flight_log << "**AIRPORTS**" << std::endl << std::endl;
        for (std::set<std::string>::iterator it = airports_list.begin (); it != airports_list.end (); ++it) {
            std::string apt = *it;
            Airport a1 = get_airport_info (apt, p.airports_db, p.frequencies_db, p.runways_db);
            flight_log << "*"<< a1.info << "*" << std::endl;
            for (unsigned i = 0; i < a1.runways.size (); ++i) {
                flight_log << a1.runways.at (i) << std::endl;
            }
            for (unsigned i = 0; i < a1.frequencies.size (); ++i) {
                flight_log << a1.frequencies.at (i) << std::endl;
            }
            flight_log << std::endl;               

            // get metars and tafs
            std::vector<std::string> res = get_metar_taf (apt, "metars");
            for (unsigned i = 0; i < res.size (); ++i) {
              metars_coll << res.at (i) << std::endl;
            }           
            res = get_metar_taf (apt, "tafs");
            for (unsigned i = 0; i < res.size (); ++i) {
              tafs_coll << res.at (i) << std::endl;
            }                        
        }        
    }

    if (navaids_list.size ()) {
        flight_log << "**NAVAIDS**" << std::endl << std::endl;
        for (std::set<std::string>::iterator it = navaids_list.begin (); it != navaids_list.end (); ++it) {
            std::string station = *it;
            std::vector<std::string> navaids_info = get_navaid_info (station, p.navaids_db);
            if (navaids_info.size () == 0) {
                std::cout << BOLDCYAN << "warning: cannot find info on navaid " << station << RESET << std::endl;
            }
            for (unsigned i = 0; i < navaids_info.size (); ++i) {
                flight_log << navaids_info.at (i) << std::endl;
            }
        }
        flight_log << std::endl;
    }

    if (metars_coll.str ().size ()) {
        flight_log << "**METARS**" << std::endl << std::endl;
        flight_log << metars_coll.str () << std::endl;
    }

    if (tafs_coll.str ().size ()) {
        flight_log << "**TAFS**" << std::endl << std::endl;
        flight_log << tafs_coll.str () << std::endl;
    }

    if (winds_list.size ()) {
        flight_log << "**WINDS/TEMPS**" << std::endl << std::endl;
        for (std::set<std::string>::iterator it = winds_list.begin (); it != winds_list.end (); ++it) {
            std::string station = *it;
            flight_log << get_winds_aloft (station) <<  std::endl;
        }
        flight_log << std::endl;
    }

    flight_log << "**INFORMATION" << std::endl << std::endl;

    flight_log << "* cruse speed: " << p.cruise_IAS << " kts" << std::endl;
    flight_log << "* average fuel per hour: " << p.fuel_per_hour << " lt/gal" << std::endl;
    flight_log << "* WCAmax (90 at 10 kts): " << p.WCAmax << " deg" << std::endl;
    flight_log << "* magnetic variation: " << fabs (p.magnetic_variation) 
        << (p.magnetic_variation > 0 ? " W" : " E") << std::endl;

    std::string airports_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airports.csv";
    std::string frequencies_db = (std::string) getenv("HOME") + (std::string) "/.fplan/airport-frequencies.csv";
    std::string runways_db = (std::string) getenv("HOME") + (std::string) "/.fplan/runways.csv";
    std::string navaids_db = (std::string) getenv("HOME") + (std::string) "/.fplan/navaids.csv";

    flight_log << "* airports DB latest: " << get_file_date (airports_db);
    flight_log << "* frequencies DB latest: " << get_file_date (frequencies_db);
    flight_log << "* runways DB latest: " << get_file_date (runways_db);
    flight_log << "* navaids DB latest: " << get_file_date (navaids_db);
    
    for (unsigned i = 0; i < p.custom.size (); ++i) {
        flight_log << "* " << p.custom.at (i) << std::endl;
    }

    return flight_log.str ();
}
#endif	// FPLAN_H  

// EOF
