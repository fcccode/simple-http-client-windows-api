@echo off
cmd /C "cd link-to-dir_lib && cd && msbuild"
cmd /C "cd regex_lib && cd && msbuild"
cmd /C "cd request_lib && cd && msbuild"
cmd /C "cd http_client && cd && msbuild"
