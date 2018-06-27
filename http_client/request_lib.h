#ifndef DLL
  #ifdef _WIN32
    #ifdef REQUEST_LIB_EXPORTS
      #ifdef __cplusplus
        #define DLL extern "C" __declspec(dllexport)
      #else
        #define DLL __declspec(dllexport)
      #endif
    #else
      #ifdef __cplusplus
        #define DLL extern "C" __declspec(dllimport)
      #else
        #define DLL __declspec(dllimport)
      #endif
    #endif
    #define DLL_CALL __cdecl
  #else
    #define DLL
    #define DLL_CALL
  #endif
#endif

#include <Shlwapi.h>
#include <wininet.h>
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "wininet.lib")

typedef struct {
  TCHAR host[255];
  TCHAR protocol[10];
  TCHAR path[2048];
  INTERNET_PORT port;
} URL_PARSE, *LPURL_PARSE;

DLL LPURL_PARSE DLL_CALL parseURL(LPCTSTR url);
DLL BOOL DLL_CALL is_https(LPCTSTR url);
DLL BOOL DLL_CALL isProtocolSupported(LPCTSTR url);
DLL INT DLL_CALL request(LPCTSTR method, LPCTSTR url, LPCTSTR postFile, LPCTSTR userAgent, LPCTSTR headers, LPCTSTR fileOut, LPTSTR* raw_resp_headers, LPINT statusCode);

