

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <codecvt>

// #include <libxml/tree.h>
// #include <libxml/HTMLparser.h>
// #include <libxml++/libxml++.h>
 
#include <curl/curl.h>
#include <regex>

using namespace std;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char*) realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}


wstring trim(const std::wstring& str, const std::wstring& newline = L"\r\n")
{
    const auto strBegin = str.find_first_not_of(newline);
    if (strBegin == std::string::npos)
        return L""; // no content

    const auto strEnd = str.find_last_not_of(newline);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

wstring HtmlToText(wstring htmlTxt) {

    std::wregex stripFormatting(L"<[^>]*(>|$)"); //match any character between '<' and '>', even when end tag is missing

    wstring s1 = std::regex_replace(htmlTxt, stripFormatting, L"");
    wstring s2 = trim(s1);
    wstring s3 = std::regex_replace(s2, std::wregex(L"\\&nbsp;"), L",");
    return s3;
}

std::wstring stringToWstring(const std::string& t_str)
{
    //setup converter
    typedef std::codecvt_utf8<wchar_t> convert_type;
    std::wstring_convert<convert_type, wchar_t> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    return converter.from_bytes(t_str);
}


int main(void) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = (char*) malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    //curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=csv&stationString=LIPY&hoursBeforeNow=1");
    curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.airnav.com/airport/KOAK");
    // curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.aviationweather.gov/windtemp/data?level=low&fcst=06&region=sfo&layout=off");
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    } else {
        /*
         * Now, our chunk.memory points to a memory block that is chunk.size
         * bytes big and contains the remote file.
         *
         * Do something nice with it!
         */

        printf("%lu bytes retrieved\n", (unsigned long)chunk.size);

      // wstringstream ttt;
      // ttt << chunk.memory;
      wstring ww (stringToWstring((string)chunk.memory));
        wcout << HtmlToText (ww) << endl << endl << endl;
        // cout << chunk.memory << endl;

        // // Parse HTML and create a DOM tree
        // xmlDoc* doc = htmlReadDoc((xmlChar*) chunk.memory, NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    
        // // Encapsulate raw libxml document in a libxml++ wrapper
        // xmlNode* r = xmlDocGetRootElement(doc);
        // xmlpp::Element* root = new xmlpp::Element(r);
    
        // // Grab the IP address
        // std::string xpath = "//*[@id=\"locator\"]/p[1]/b/font/text()";
        // auto elements = root->find(xpath);
        // std::cout << "Your IP address is:" << std::endl;
        // std::cout << dynamic_cast<xmlpp::ContentNode*>(elements[0])->get_content() << std::endl;
    
        // delete root;
        // xmlFreeDoc(doc);
       }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    free(chunk.memory);

    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();

    return 0;
}