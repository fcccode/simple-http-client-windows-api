/**
 * Module này có nhiệm vụ tạo http request lấy nội dung trang web, lưu vào file trong hệ thống, đọc headers và status code
 */

#include <windows.h>
#include <tchar.h>

#include "request_lib.h"

// tách lấy các thành phần của url
DLL LPURL_PARSE DLL_CALL parseURL(LPCTSTR url){
  URL_COMPONENTS urlcomponents;
  LPURL_PARSE url_parse = (LPURL_PARSE)malloc(sizeof(URL_PARSE));
  
  urlcomponents.dwStructSize=sizeof(URL_COMPONENTS);
  urlcomponents.lpszHostName=url_parse->host;
  urlcomponents.dwHostNameLength=255;
  urlcomponents.lpszScheme=url_parse->protocol;
  urlcomponents.dwSchemeLength=10;
  urlcomponents.dwPasswordLength=0;
  urlcomponents.lpszPassword=NULL;
  urlcomponents.dwUrlPathLength=2048;
  TCHAR UrlPath[2048];
  urlcomponents.lpszUrlPath=UrlPath;
  urlcomponents.dwUserNameLength=0;
  urlcomponents.lpszUserName=NULL;
  urlcomponents.dwExtraInfoLength=2048;
  TCHAR ExtraInfo[2048];
  urlcomponents.lpszExtraInfo=ExtraInfo;

  if(InternetCrackUrl(url,_tcslen(url), 0, &urlcomponents)){
    *(url_parse->path) = 0;
    _tcsncat_s(url_parse->path, _countof(url_parse->path), UrlPath, _TRUNCATE);
    _tcsncat_s(url_parse->path, _countof(url_parse->path), ExtraInfo, _TRUNCATE);

    url_parse->port = urlcomponents.nPort;
    return url_parse;
  }
  free(url_parse);
  return NULL;
}

// kiểm tra xem url có phải https
DLL BOOL DLL_CALL is_https(LPCTSTR url){
  LPCTSTR https = _T("https://");
  if(!_tcsncmp(https, url, _tcslen(https)))
    return TRUE;
  return FALSE;
}

// kiểm tra giao thức có hỗ trợ ko
DLL BOOL DLL_CALL isProtocolSupported(LPCTSTR url){
  LPCTSTR http = _T("http://");
  if(!_tcsncmp(http, url, _tcslen(http))){
    return TRUE;
  }
  LPCTSTR https = _T("https://");
  if(!_tcsncmp(https, url, _tcslen(https))){
    return TRUE;
  }
  return FALSE;
}

// lấy cụ thể 1 header
LPTSTR getHeader(HINTERNET hRequest, LPCTSTR headerName){
  DWORD dwSize = 20;
  LPTSTR lpOutBuffer = (LPTSTR)malloc(sizeof(TCHAR)*dwSize);
  *lpOutBuffer = 0;
  _stprintf_s(lpOutBuffer, dwSize, headerName);
  retry:
  if(!HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM, (LPVOID)lpOutBuffer,&dwSize,NULL)){
    if (GetLastError()==ERROR_HTTP_HEADER_NOT_FOUND){
      free(lpOutBuffer);
      return NULL;
    }else{
      if (GetLastError()==ERROR_INSUFFICIENT_BUFFER){
        free(lpOutBuffer);
        lpOutBuffer = (LPTSTR)calloc(dwSize, sizeof(TCHAR));
        _stprintf_s(lpOutBuffer, dwSize, headerName);
        goto retry;
      }else{
        free(lpOutBuffer);
        return NULL;
      }
    }
  }
  return lpOutBuffer;
}

// lấy status code
INT getHTTPStatusCode(HINTERNET hRequest, LPINT status){
  TCHAR statusText[20]; // change to wchar_t for unicode
  DWORD dwSize = sizeof(statusText);
  if(!HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, &statusText, &dwSize, NULL))
    return 1;
  *status = _tstoi(statusText);
  return 0;
}

// lấy toàn bộ header
BOOL getRawHeaders(HINTERNET hRequest, LPTSTR* raw_headers){
  LPTSTR lpOutBuffer=NULL;
  DWORD dwSize = 0;
  retry:
  if(!HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, (LPVOID)lpOutBuffer, &dwSize, NULL)){
    if (GetLastError()==ERROR_HTTP_HEADER_NOT_FOUND){
      *raw_headers = NULL;
      return TRUE;
    }else{
      if (GetLastError()==ERROR_INSUFFICIENT_BUFFER){
        lpOutBuffer = (LPTSTR)calloc(dwSize, sizeof(CHAR));
        goto retry;
      }else{
        if(lpOutBuffer){
         free(lpOutBuffer);
        }
        *raw_headers = NULL;
        return FALSE;
      }
    }
  }
  *raw_headers = (LPTSTR)lpOutBuffer;
  return TRUE;
}


// thực hiện kết nối gửi header, post, lấy nội dung, ghi file, đọc header và status code trong 1 hàm
DLL INT DLL_CALL request(LPCTSTR method, LPCTSTR url, LPCTSTR postFile, LPCTSTR userAgent, LPCTSTR headers, LPCTSTR fileOut, LPTSTR* raw_resp_headers, LPINT statusCode){
  // Open internet session
  HANDLE hOpen=InternetOpen(userAgent, // user agent
    INTERNET_OPEN_TYPE_PRECONFIG, // access type
    NULL, // proxy
    NULL, // proxy by pass
    0); // flags
  if(hOpen == INVALID_HANDLE_VALUE){
    return GetLastError() ? GetLastError() : -1;
  }

  LPURL_PARSE url_parse = parseURL(url);

  HANDLE hConnection=InternetConnect(hOpen,
    url_parse->host, (url_parse->port ? url_parse->port : (is_https(url) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT)),
    NULL, NULL,
    INTERNET_SERVICE_HTTP,
    0,
    (DWORD_PTR)NULL);
  if(hConnection == INVALID_HANDLE_VALUE){
    InternetCloseHandle(hOpen);
    free(url_parse);
    return GetLastError() ? GetLastError() : -2;
  }
  
  LPBYTE pBuf = NULL;
  DWORD64 dwLen = 0;
  LPVOID omp;
  LPVOID cmp;
  HANDLE fpPostFile;
  if(postFile){
    fpPostFile = CreateFile(postFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if(fpPostFile == INVALID_HANDLE_VALUE){
      InternetCloseHandle(hConnection);
      InternetCloseHandle(hOpen);
      free(url_parse);
      return GetLastError() ? GetLastError() : -3;
    }
    
    // GetFileSize
    LARGE_INTEGER liSize;
    liSize.LowPart = GetFileSize(fpPostFile, &liSize.HighPart);

    SIZE_T sizeToMap = 0; // 0 to map all file
    cmp = CreateFileMapping(fpPostFile, NULL, PAGE_READWRITE, 0, sizeToMap, _T("Local\\http_post_file"));
    if(cmp == NULL){
      InternetCloseHandle(hConnection);
      InternetCloseHandle(hOpen);
      free(url_parse);
      CloseHandle(fpPostFile);
      return GetLastError() ? GetLastError() : -4;
    }
    omp = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, _T("Local\\http_post_file"));
    if(omp == NULL){
      InternetCloseHandle(hConnection);
      InternetCloseHandle(hOpen);
      free(url_parse);
      CloseHandle(fpPostFile);
      return GetLastError() ? GetLastError() : -5;
    }
     
    pBuf = (LPBYTE) MapViewOfFile(omp, FILE_MAP_ALL_ACCESS, 0, 0, sizeToMap);
    dwLen = liSize.QuadPart;
  }

  LPCTSTR szAcceptTypes[] = {_T("*/*"), NULL};
  HANDLE hRequest=HttpOpenRequest( hConnection,
    method, // HTTP Verb
    url_parse->path, // Object Name
    _T("HTTP/1.1"), // Version
    url, // Referer
    szAcceptTypes, // Accept Type  
    INTERNET_FLAG_NO_COOKIES |
    INTERNET_FLAG_NO_AUTO_REDIRECT |
    INTERNET_FLAG_RELOAD |
    INTERNET_FLAG_DONT_CACHE |
    // INTERNET_FLAG_KEEP_CONNECTION |
    INTERNET_FLAG_NO_CACHE_WRITE |
    INTERNET_FLAG_FORMS_SUBMIT |
    (is_https(url) ? INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_SECURE : 0),
    (DWORD_PTR)NULL); // context call-back point
  if(hRequest == INVALID_HANDLE_VALUE){
    InternetCloseHandle(hConnection);
    InternetCloseHandle(hOpen);
    free(url_parse);
    CloseHandle(fpPostFile);
    return GetLastError() ? GetLastError() : -6;
  }

  BOOL isSent = HttpSendRequest(hRequest, headers, _tcslen(headers), pBuf, (DWORD)dwLen);
  if(postFile){
    UnmapViewOfFile(pBuf);
    CloseHandle(cmp);
    CloseHandle(omp);
    CloseHandle(fpPostFile);
  }
  if(!isSent){
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnection);
    InternetCloseHandle(hOpen);
    free(url_parse);
    return GetLastError() ? GetLastError() : -7;
  }
  /*
  get any header here
  */
  if(raw_resp_headers)
    getRawHeaders(hRequest, raw_resp_headers);
  if(statusCode)
    getHTTPStatusCode(hRequest, statusCode);
  // _tprintf(_T("\r\n%s\r\n"), getHeader(hRequest, _T("Content-Type")));
  // _tprintf(_T("\r\n%s\r\n"), getHeader(hRequest, _T("Content-Length")));
  if(PathFileExists(fileOut))
    DeleteFile(fileOut);
  HANDLE fp = CreateFile(fileOut, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if(fp == INVALID_HANDLE_VALUE){
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnection);
    InternetCloseHandle(hOpen);
    free(url_parse);
    return GetLastError() ? GetLastError() : -8;
  }
  DWORD dwBytesRead;
  DWORD buff_len = 4096;
  do{
    LPBYTE lpBuffer = (LPBYTE)calloc(buff_len, sizeof(BYTE));
    if(!InternetReadFile(hRequest, (LPVOID)lpBuffer, buff_len-1, &dwBytesRead)){
      CloseHandle(fp);
      InternetCloseHandle(hRequest);
      InternetCloseHandle(hConnection);
      InternetCloseHandle(hOpen);
      free(lpBuffer);
      free(url_parse);
      return GetLastError() ? GetLastError() : -9;
    }
    // _tprintf(_T("%d\r\n"), dwBytesRead);
    if(dwBytesRead){
      DWORD written;
      if(!WriteFile(fp, lpBuffer, dwBytesRead, &written,NULL)){
        CloseHandle(fp);
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnection);
        InternetCloseHandle(hOpen);
        free(url_parse);
        free(lpBuffer);
        return GetLastError() ? GetLastError() : -10;
      }
    }
    free(lpBuffer);
  }while(dwBytesRead);
  CloseHandle(fp);
  InternetCloseHandle(hRequest);
  InternetCloseHandle(hConnection);
  InternetCloseHandle(hOpen);
  free(url_parse);
  return 0;
}

