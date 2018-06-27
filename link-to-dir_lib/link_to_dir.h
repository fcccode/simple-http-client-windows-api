#ifndef DLL
  #ifdef _WIN32
    #ifdef LINK_TO_DIR_LIB_EXPORTS
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

DLL INT DLL_CALL complete_url(LPCTSTR baseUrl, const char* path, LPTSTR outUrl);
DLL INT DLL_CALL tao_cay_thu_muc(LPCTSTR output, LPCTSTR link, LPTSTR outpath);
