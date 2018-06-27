#ifndef DLL
  #ifdef _WIN32
    #ifdef REGEX_LIB_EXPORTS
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

#ifdef __cplusplus
extern "C" {
#endif
  typedef struct {
    void** arr;
    unsigned int len;
  } ARRAY;
#ifdef __cplusplus
} // extern "C"
#endif

DLL BOOL DLL_CALL is_html(char* raw_header);
DLL INT DLL_CALL process_html_file_replace(LPCTSTR output, LPCTSTR baseUrl, LPCTSTR html_file, LPCTSTR out_file, ARRAY *a);
