/**
 * Module này có nhiệm vụ đọc file html, tìm kiếm tất cả các link resources, thay thế các link đó bằng link file
 */
#include <string>
#include <iostream>
#include <sstream>
#include <regex>

#include <windows.h>
#include <tchar.h>

// regex_replace callback
// https://stackoverflow.com/questions/22617209/regex-replace-with-callback-in-c11
// clang++ -std=c++11 -stdlib=libc++ -o test test.cpp
namespace std {

template<class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last,
  const std::basic_regex<CharT,Traits>& re, UnaryFunction f) {
  std::basic_string<CharT> s;

  typename std::match_results<BidirIt>::difference_type
    positionOfLastMatch = 0;
  auto endOfLastMatch = first;

  auto callback = [&](const std::match_results<BidirIt>& match) {
    auto positionOfThisMatch = match.position(0);
    auto diff = positionOfThisMatch - positionOfLastMatch;

    auto startOfThisMatch = endOfLastMatch;
    std::advance(startOfThisMatch, diff);

    s.append(endOfLastMatch, startOfThisMatch);
    s.append(f(match));

    auto lengthOfMatch = match.length(0);

    positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

    endOfLastMatch = startOfThisMatch;
    std::advance(endOfLastMatch, lengthOfMatch);
  };

  std::regex_iterator<BidirIt> begin(first, last, re), end;
  std::for_each(begin, end, callback);

  s.append(endOfLastMatch, last);

  return s;
}

template<class Traits, class CharT, class UnaryFunction>
std::string regex_replace(const std::string& s,
  const std::basic_regex<CharT,Traits>& re, UnaryFunction f) {
  return regex_replace(s.cbegin(), s.cend(), re, f);
}

} // namespace std

// regex_replace callback

#include "regex_lib.h"
#define BUFF_SIZE 3072
#define REGEX_TAG_PARSER R"(<link[^>]+?type\s*=\s*["']text\/css["'][^>]*?>|<link[^>]+?rel\s*=\s*["']stylesheet["'][^>]*?>|<(img|script)[^>]+?src\s*=\s*(\"|')(?!data:)[^>]*?\2[^>]*?>)"

extern "C" {
  #include "html-entities-lib/entities.h"

  #include "link_to_dir.h"
  #pragma comment (lib, "link_to_dir.lib")
}

// lấy tag cuối cùng chưa hoàn thiện của buffer
const char* getLastTag(const char* html, unsigned int html_len){
  if(!html_len) return NULL;
  
  for(unsigned int i=html_len-1; i>=0; i--){
    if(html[i] == '>'){
      return NULL;
    }
    if(html[i] == '<'){
      return html+i;
    }
  }
  return NULL;
}

// check xem phần tử có trong array chưa
bool in_array(const char* e, ARRAY* a){
  for(unsigned int i=0; i<(a->len); i++){
    if(!strcmp(e, (char*)a->arr[i]))
      return true;
  }
  return false;
}

// check xem header có content-type là html ko
DLL BOOL DLL_CALL is_html(char* raw_header){
  std::string str(raw_header);
  std::regex r{R"(content-type:\s*text/x?html)", std::regex_constants::icase};
  std::smatch m;
  if(std::regex_search(str, m, r)){
    return TRUE;
  }
  return FALSE;
}


// hàm thay thế link của resources trong buffer
std::string &process_html_replace(LPCTSTR output, LPCTSTR baseUrl, char* html, ARRAY *a){
  std::string str(html);
  std::regex r{REGEX_TAG_PARSER, std::regex_constants::icase};
  std::smatch m;
  
  std::string replacement = regex_replace(str, r, [&a, &baseUrl, &output](const std::smatch& m){
    std::regex sub_r{R"((src|href)\s*=\s*("|')(.+?)\2)", std::regex_constants::icase};
    std::string sub_str = m[0].str();
    return regex_replace(sub_str, sub_r, [&a, &baseUrl, &output](const std::smatch& sub_m){
      std::string sub_m_0 = sub_m[0].str();
      std::string match_str = sub_m[3].str();
      std::string quot = sub_m[2].str();
      const char* match = match_str.c_str();
      size_t m_len = strlen(match)+1;
      if(m_len > 2000) return sub_m[0].str();
      char* temp = (char*)malloc(m_len*sizeof(char));
      decode_html_entities_utf8(temp, match);
      match = temp;
      TCHAR subUrl[2000];
      if(complete_url(baseUrl, match, subUrl)){
        free(temp);
        return sub_m[0].str();
      }
      if(!in_array(match, a)){
        unsigned int index = a->len;
        a->len++;
        a->arr[index] = (void*)match;
      }else{
        free(temp);
      }
      TCHAR fileOut[2000];
      if(tao_cay_thu_muc(output, subUrl, fileOut)){
        return sub_m[0].str();
      }
      const DWORD sizeWideChar = sizeof(CHAR)*_tcslen(fileOut)*3+1;
      CHAR fileOutChar[2000];
      WideCharToMultiByte(CP_UTF8, 0, fileOut, -1, fileOutChar, sizeWideChar, NULL, NULL);
      
      char prefix[MAX_PATH] = {0};
      BOOL start = false;
      for(DWORD i=0; i<_tcslen(baseUrl); i++){
        if(baseUrl[i] == _T('?')) break;
        if(baseUrl[i] == _T('/') && baseUrl[i+1] == _T('/')){
          start = TRUE;
          continue;
        }
        if(!start) continue;
        if(baseUrl[i] == _T('/'))
          strcat_s(prefix, _countof(prefix), "..\\");
      }
      
      return std::string(sub_m[1].str()+"="+quot+prefix+fileOutChar+quot);
    }); // sub replace
  });

  std::string* replace_c = new std::string(replacement.c_str());
  return *replace_c; // main replace
}

// hàm đọc file, ghi file đã được thay thế link
DLL INT DLL_CALL process_html_file_replace(LPCTSTR output, LPCTSTR baseUrl, LPCTSTR html_file, LPCTSTR out_file, ARRAY *a){
  FILE* fp;
  if(_tfopen_s(&fp, html_file, _T("rb")))
    return 1;
  FILE* fw;
  if(_tfopen_s(&fw, out_file, _T("wb")))
    return 2;
  try{
    a->arr = (void**)malloc(sizeof(char*)*2018);
    a->len = 0;
    if(!fp)
      return FALSE;
    size_t conti;
    const unsigned int html_size = BUFF_SIZE*2+1024;
    char html_buff[html_size] = {0};
    int lastSkip = 0;
    while(1){
      char buff[BUFF_SIZE+1] = {0};
      conti = fread(buff, BUFF_SIZE, 1, fp);
      sprintf_s(html_buff, html_size, "%s%s", html_buff, buff);
      
      unsigned int html_buff_len = strlen(html_buff);

      std::string &replace_html = process_html_replace(output, baseUrl, html_buff, a);

      if(lastSkip)
        fseek(fw, -lastSkip, SEEK_CUR);
      size_t replace_len = replace_html.length();
      fwrite(replace_html.c_str(), replace_len, 1, fw);
      
      const char* conti_buff = getLastTag(replace_html.c_str(), replace_len);
      if(conti_buff && (lastSkip = strlen(conti_buff)) < 2048){
        sprintf_s(html_buff, html_size, "%s", conti_buff);
      }else{
        lastSkip = 0;
        *html_buff = 0;
      }
      
      if(feof(fp)){
        break;
      }
    }
    fclose(fp);
    fclose(fw);
  }catch (...){
    fclose(fp);
    fclose(fw);
    return FALSE;
  }
  return TRUE;
}
