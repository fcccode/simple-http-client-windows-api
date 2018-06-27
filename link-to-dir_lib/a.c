/**
 * Module này có nhiệm vụ hoàn thành đường dẫn tương đối trong html thành đường dẫn tuyệt đối, tạo cây thư mục từ đường link
 */

#include <windows.h>
#include <tchar.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>

#ifdef UNICODE
#include <locale.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

#include <Shlwapi.h>
#include <wininet.h>
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "wininet.lib")

#include "link_to_dir.h"

// check url có phải https
BOOL is_https(LPCTSTR url){
  LPCTSTR https = _T("https://");
  if(!_tcsncmp(https, url, _tcslen(https)))
    return TRUE;
  return FALSE;
}

// resolve url về absolution path
DLL INT DLL_CALL complete_url(LPCTSTR baseUrl, const char* path, LPTSTR outUrl){
  LPTSTR subUrl;
  
  DWORD wLen = strlen(path);
  TCHAR lpWideChar[2000];
  MultiByteToWideChar(CP_UTF8, 0, path, -1, lpWideChar, wLen);
  lpWideChar[wLen] = 0;

  subUrl = lpWideChar;

  if(!_tcsncicmp(subUrl, _T("about:"), 6) || !_tcsncicmp(subUrl, _T("data:"), 5) || !_tcsncicmp(subUrl, _T("mailto:"), 7) || !_tcsncicmp(subUrl, _T("javascript:"), 10) || !_tcsncicmp(subUrl, _T("http://"), 7) || !_tcsncicmp(subUrl, _T("https://"), 8)){
    _stprintf_s(outUrl, 2000, _T("%s"), subUrl);
    return 0;
  }
  if(!_tcsncicmp(subUrl, _T("//"), 2)){
    TCHAR temp[2000];
    _stprintf_s(temp, 2000, _T("%s:%s"), is_https(baseUrl) ? _T("https") : _T("http"), subUrl);
    subUrl = temp;
    _stprintf_s(outUrl, 2000, _T("%s"), subUrl);
    return 0;
  }

  TCHAR combineUrl[2000];
  wLen = 2000;
  if(!InternetCombineUrl(baseUrl, subUrl, combineUrl, &wLen, ICU_BROWSER_MODE)){
    _stprintf_s(outUrl, 2000, _T("%s"), _T(""));
    return GetLastError();
  }
  combineUrl[wLen] = '\0';
  _stprintf_s(outUrl, 2000, _T("%s"), combineUrl);

  wLen = 2000;
  combineUrl[0] = 0;
  if(!InternetCanonicalizeUrl(outUrl, combineUrl, &wLen, ICU_BROWSER_MODE)){
    _stprintf_s(outUrl, 2000, _T("%s"), _T(""));
    return GetLastError();
  }
  combineUrl[wLen] = '\0';
  _stprintf_s(outUrl, 2000, _T("%s"), combineUrl);
  return 0;
}

// kiểm tra xem thư mục tồn tại ko
BOOL checkDirectoryExists(LPCTSTR path){
  DWORD attr = GetFileAttributes(path);
  return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}
/**
 * output : thư mục default
 * link : đường link cần dựng cây thư mục
 * outpath : trả về thư mục đã tạo
 * @return error code | 0
 */
DLL INT DLL_CALL tao_cay_thu_muc(LPCTSTR output, LPCTSTR link, LPTSTR outpath){
  DWORD wLen = 2000;
  TCHAR _link[2000];
  _link[0] = 0;
  if(InternetCanonicalizeUrl(link, _link, &wLen, ICU_DECODE|ICU_NO_ENCODE )){
    _link[wLen] = '\0';
    link = (LPTSTR)_link;
  }
  outpath[0] = 0;
  // tạo cây thư mục
  BOOL notfound = TRUE;
  for(DWORD i=0; i < _tcslen(link)-1; i++){
    if(link[i] == _T('/') && link[i+1] == _T('/')){
      if(notfound){
        link = link+i+2;
        notfound = FALSE;
      }
    }
  }
  if(notfound){
    return 1;
  }
  TCHAR path[2000] = {0};
  _stprintf_s(path, 2000, _T("%s/%s"), output, link);
  DWORD pathLen = _tcslen(path);
  for(DWORD i=0; i<pathLen; i++)
    if(path[i] == _T('?')){
      path[i] = 0;
      break;
    }
  pathLen = _tcslen(path);
  
  // thêm index.html nếu là dir
  if(path[pathLen-1] == _T('/')) _tcscat_s(path, 2000, _T("index.html"));
  pathLen = _tcslen(path);

  LPTSTR lastPos = path;
  for(DWORD i=0; i<pathLen; i++){
    if(path[i] == _T('\\')) path[i] = _T('/');
    if(path[i] != _T('/')) continue;
    path[i] = 0;
    // mkdir here
    // CON, PRN, AUX, NUL, COM0, COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9
    if(!_tcsicmp(lastPos, _T("CON")) || !_tcsicmp(lastPos, _T("PRN")) || !_tcsicmp(lastPos, _T("AUX")) || !_tcsicmp(lastPos, _T("NUL")) ||
    ((!_tcsncicmp(lastPos, _T("COM"), 3) || !_tcsncicmp(lastPos, _T("LPT"), 3)) && lastPos[3] <= _T('9') && lastPos[3] >= _T('0') && lastPos[4] == 0)
    ){
      lastPos[1] = _T('0');
    }
    if(!checkDirectoryExists(path)){
      if(!CreateDirectory(path, NULL)){
        return GetLastError();
      }
    }
    _tcscat_s(outpath, 2000, lastPos);
    _tcscat_s(outpath, 2000, _T("\\"));
    lastPos = path+i+1;
    path[i] = _T('\\');
  }
  _tcscat_s(outpath, 2000, lastPos);
  
  
  return 0;
}

