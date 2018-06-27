@echo off
cmd /C "cd link-to-dir_lib && cd && msbuild /t:Clean"
cmd /C "cd regex_lib && cd && msbuild /t:Clean"
cmd /C "cd request_lib && cd && msbuild /t:Clean"
cmd /C "cd http_client && cd && msbuild /t:Clean"
