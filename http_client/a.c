#include <windows.h>
#include <string.h>
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

#pragma comment (lib, "regex_lib.lib")
#include "regex_lib.h"

#pragma comment (lib, "request_lib.lib")
#include "request_lib.h"

#pragma comment (lib, "link_to_dir.lib")
#include "link_to_dir.h"

BOOL getHttpStatus(INT statusCode, LPTSTR http_status);

// Hàm lấy tham số truyền vào từ commandline
LPCTSTR getParams(LPCTSTR argv, LPCTSTR name){
  if(!_tcsncmp(name, argv, _tcslen(name))){
    return argv+_tcslen(name);
  }
  return NULL;
}

// http request thread struct
typedef struct {
  INT id; // thread id
  LPCTSTR method;
  TCHAR url[2000];
  LPCTSTR postFile;
  LPCTSTR userAgent;
  TCHAR request_headers[2048];
  TCHAR fileOut[MAX_PATH];
  LPTSTR* raw_headers;
  INT statusCode;
} THREAD_PARAM, *LPTHREAD_PARAM;

// thread download các thành phần trong thư mục
DWORD WINAPI download_thread(LPVOID lpParam){
  LPTHREAD_PARAM param = (LPTHREAD_PARAM)lpParam;
  INT requestErrorCode = request(param->method, param->url, param->postFile, param->userAgent, param->request_headers, param->fileOut, param->raw_headers, &param->statusCode);
  if(requestErrorCode)
    _endthreadex(requestErrorCode);
  _endthreadex(0);
  return 0;
}

int _tmain(DWORD argc, LPTSTR argv[]){
#ifdef UNICODE
  _setmode(_fileno(stdout), _O_U8TEXT /* _O_U16TEXT */);
  // (!setlocale(LC_CTYPE, "") || !setlocale(LC_ALL, ""));
#endif

  if(argc < 2){
    _tprintf(_T("\r\nUsage: \"%s\" --url={URL} [--method=GET|PUT|POST|PATCH|HEAD|OPTIONS] [--user-agent={USER_AGENT}] [-H={RequestHeaderName: Value}] [--post={POST_DATA}] [--post-file={FILE_PATH}] [--output=={OUTPUT_DIR}] [--max-threads=={MAX_THREADS}]\r\n"), argv[0]);
    return 0;
  }

  // GetParams
  TCHAR headers[2048];
  *headers = 0;
  _tcsncat_s(headers, _countof(headers), _T("Accept-Language: en-US,en;q=0.5\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n"), _TRUNCATE);
  LPCTSTR url = NULL;
  LPCTSTR output = NULL;
  LPCTSTR postData = NULL;
  LPCTSTR postFile = NULL;
  LPCTSTR method = NULL;
  LPCTSTR userAgent = NULL;
  INT maxThreads = 0;
  for(DWORD i=0; i<argc; i++){
    if(!postFile)
      postFile = getParams(argv[i], _T("--post-file="));
    if(!userAgent)
      userAgent = getParams(argv[i], _T("--user-agent="));
    if(!method)
      method = getParams(argv[i], _T("--method="));
    if(!postData)
      postData = getParams(argv[i], _T("--post="));
    if(!output)
      output = getParams(argv[i], _T("--output="));
    if(!output)
      output = getParams(argv[i], _T("-O="));
    if(!url)
      url = getParams(argv[i], _T("--url="));
    if(!url)
      url = getParams(argv[i], _T("-U="));
    
    LPCTSTR maxThreadsCh = getParams(argv[i], _T("--max-threads="));
    if(maxThreadsCh && !maxThreads)
      maxThreads = _tstoi(maxThreadsCh);
    LPCTSTR header = getParams(argv[i], _T("-H="));
    if(header){
      _tcsncat_s(headers, _countof(headers), header, _TRUNCATE);
      _tcsncat_s(headers, _countof(headers), _T("\r\n"), _TRUNCATE);
    }
  }
  if(maxThreads < 1) maxThreads = 1;
  if(output == NULL) output = _T(".\\output");
  headers[_tcslen(headers)-2] = 0;
  // _tprintf(_T("\r\nRequest headers: '%s'\r\n"), headers);
  // end GetParams

  // check and validate params
  if(!url){
    _tprintf(_T("\r\nPlease provide --url option.\r\n"));
    return 1;
  }

  LPURL_PARSE url_parse = parseURL(url);
  if(url_parse == NULL){
    _tprintf(_T("\r\n--url: \"%s\" parse failed.\r\n"), url);
    return 1;
  }else
    free(url_parse);
  

  if(!isProtocolSupported(url)){
    _tprintf(_T("\r\n--url: \"%s\" protocol not supported.\r\n"), url);
    return 1;
  }

  if(!userAgent)
    userAgent = _T("Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.79 Safari/537.36");

  DWORD postLen = 0;
  if(postData)
    postLen = _tcslen(postData);
  // trick
  LPTSTR lpFileName = NULL;
  if(postLen){
    srand((DWORD)time(NULL));
    lpFileName = (LPTSTR)calloc(MAX_PATH, sizeof(TCHAR));
    _stprintf_s(lpFileName, MAX_PATH, _T("./temp_to_post_%d.txt"), rand());
    if(PathFileExists(lpFileName)){
      DeleteFile(lpFileName);
    }
    HANDLE fp = CreateFile(lpFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if(fp == INVALID_HANDLE_VALUE){
      _tprintf(_T("\r\n--post: CreateFile error: %d\r\n"), GetLastError());
      free(lpFileName);
      return 1;
    }
    DWORD written;
    DWORD sizeChar = sizeof(CHAR)*postLen*3+1;
    // _tprintf(_T("%d\r\n"), sizeChar);
    LPSTR utf8 = (LPSTR)malloc(sizeChar);
    WideCharToMultiByte(CP_UTF8, 0, postData, -1, utf8, sizeChar, NULL, NULL);
    if(!WriteFile(fp, utf8, strlen(utf8), &written,NULL)){
      _tprintf(_T("\r\n--post: WriteFile error: %d"), GetLastError());
      CloseHandle(fp);
      DeleteFile(lpFileName);
      free(lpFileName);
      return 1;
    }
    free(utf8);
    CloseHandle(fp);
  }
  postFile = lpFileName;

  if(postFile){
    if(!PathFileExists(postFile)){
      _tprintf(_T("\r\n--post-file: file \"%s\" not exists.\r\n"), postFile);
      return 1;
    }
    if(!method){
      method = _T("POST");
    }
  }
  if(!method)
    method = _T("GET");

  LPTSTR raw_headers = NULL;
  TCHAR myAbsUrl[2001];
  DWORD myAbsUrlLen=2000;
  if(!InternetCanonicalizeUrl(url, myAbsUrl, &myAbsUrlLen, ICU_BROWSER_MODE)){
    _tprintf(_T("InternetCanonicalizeUrl error: %d"), GetLastError());
    if(postLen)
      DeleteFile(lpFileName);
    return 1;
  }
  // end check and validate params
  
  // request đến site
  TCHAR filePath[MAX_PATH];
  DWORD ret = tao_cay_thu_muc(output, myAbsUrl, filePath);
  _tprintf(_T("Requesting site \"%s\" ..\r\n"), myAbsUrl);
  INT statusCode = 0;
  INT requestErrorCode = request(method, myAbsUrl, postFile, userAgent, headers, filePath, &raw_headers, &statusCode);
  _tprintf(_T("File saved: %s\r\n"), filePath);
  TCHAR http_status[100];
  getHttpStatus(statusCode, http_status);
  _tprintf(_T("Status: %d %s\r\n"), statusCode, http_status);
  if(requestErrorCode){
    if(raw_headers){
      free(raw_headers);
    }
    _tprintf(_T("\r\nRequest error code: %d\r\n"), requestErrorCode);
    if(postLen)
      DeleteFile(lpFileName);
    return 1;
  }
  if(!raw_headers){
    _tprintf(_T("\r\nCant get http response headers.\r\n"));
    if(postLen)
      DeleteFile(lpFileName);
    return 1;
  }

  // is html?
  DWORD sizeWideChar = sizeof(CHAR)*_tcslen(raw_headers)*3+1;
  LPSTR headersUtf8 = (LPSTR)malloc(sizeWideChar);
  WideCharToMultiByte(CP_UTF8, 0, raw_headers, -1, headersUtf8, sizeWideChar, NULL, NULL);
  BOOL IS_HTML = is_html(headersUtf8);
  free(headersUtf8);

  free(raw_headers);
  
  if(IS_HTML) {

    // Regex bắt và thay thế link
    ARRAY queue;
    TCHAR fileOut[MAX_PATH];
    _stprintf_s(fileOut, MAX_PATH, _T("%s%s"), filePath, _T(".out.html"));
    process_html_file_replace(output, myAbsUrl, filePath, fileOut, &queue);
    
    if(queue.len == 0){
      if(postLen)
        DeleteFile(lpFileName);
      return 0;
    }

    // thread download resources
    if(queue.len < (DWORD)maxThreads) maxThreads = queue.len;
    LPTHREAD_PARAM lpParam = (LPTHREAD_PARAM)calloc(maxThreads, sizeof(THREAD_PARAM));
    LPHANDLE hThreadArray = (LPHANDLE)calloc(maxThreads, sizeof(HANDLE));
    _tprintf(_T("\r\nMax threads: %d\r\n"), maxThreads);
    DWORD nextQueue = 0;
    for(INT i = 0; i < maxThreads; i++){
      char* str = (char*)queue.arr[i];
      nextQueue = i+1;
      
      if(complete_url(url, str, lpParam[i].url) == -1){
        // skip
        free(str);
        continue;
      }

      DWORD ret = tao_cay_thu_muc(output, lpParam[i].url, lpParam[i].fileOut);
      if(ret){
        _tprintf(_T("\r\ntao_cay_thu_muc error: %d\r\n"), ret);
        continue;
      }
      _tprintf(_T("\r\nDownloading: %s ==> %s\r\n"), lpParam[i].url, lpParam[i].fileOut);
      *lpParam[i].request_headers = 0;
      _tcsncat_s(lpParam[i].request_headers, _countof(lpParam[i].request_headers), _T("Accept-Language: en-US,en;q=0.5\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n"), _TRUNCATE);
      lpParam[i].raw_headers = NULL;
      lpParam[i].userAgent = userAgent;
      lpParam[i].postFile = NULL;
      lpParam[i].method = _T("GET");
      hThreadArray[i] = (HANDLE)_beginthreadex(NULL, 0, download_thread, (LPVOID)&lpParam[i], 0, &lpParam[i].id);
      if (hThreadArray[i] == NULL) {
        _tprintf(_T("\r\n_beginthreadex() error: %u, thread nth: %u"), GetLastError(), i);
        continue;
      }

      free((char*)str);
    }
    
    // đợi từng thread download xong, add thread mới vào
    BOOL noBreakWhile = TRUE;
    do{
      WaitForMultipleObjects(maxThreads, hThreadArray, FALSE, INFINITE);
      for(INT i=0; i<maxThreads; i++){
        DWORD exitCode;
        if(!GetExitCodeThread(hThreadArray[i], &exitCode)) continue;
        if(exitCode != STILL_ACTIVE){
          // thread đã kết thúc
          _tprintf(_T("Thread %d ended, code: %d\r\n"), i, exitCode);
          if(nextQueue < queue.len){
            char* str = (char*)queue.arr[nextQueue];
            nextQueue++;
            
            if(complete_url(url, str, lpParam[i].url) == -1){
              // skip
              free(str);
              continue;
            }

            DWORD ret = tao_cay_thu_muc(output, lpParam[i].url, lpParam[i].fileOut);
            if(ret){
              _tprintf(_T("\r\ntao_cay_thu_muc error: %d\r\n"), ret);
              continue;
            }
            _tprintf(_T("\r\nDownloading: %s ==> %s\r\n"), lpParam[i].url, lpParam[i].fileOut);
            *lpParam[i].request_headers = 0;
            _tcsncat_s(lpParam[i].request_headers, _countof(lpParam[i].request_headers), _T("Accept-Language: en-US,en;q=0.5\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n"), _TRUNCATE);
            lpParam[i].raw_headers = NULL;
            lpParam[i].userAgent = userAgent;
            lpParam[i].postFile = NULL;
            lpParam[i].method = _T("GET");
            hThreadArray[i] = (HANDLE)_beginthreadex(NULL, 0, download_thread, (LPVOID)&lpParam[i], 0, &lpParam[i].id);
            if (hThreadArray[i] == NULL) {
              _tprintf(_T("\r\n_beginthreadex() error: %u, thread nth: %u"), GetLastError(), i);
              continue;
            }

            free((char*)str);
          } // nextQueue
          else{
            noBreakWhile = FALSE;
          }
        }
      }
    }while(noBreakWhile);
    
    // Đợi tất cả thread kết thúc
    WaitForMultipleObjects(maxThreads, hThreadArray, TRUE, INFINITE);
    free(queue.arr);

  } // is_html
  
  // xoá file post
  if(postLen)
    DeleteFile(lpFileName);
  
  return 0;
} // _tmain
