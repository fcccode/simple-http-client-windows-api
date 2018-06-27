#include <windows.h>
#include <string.h>
#include <tchar.h>
#include <stdlib.h>

BOOL getHttpStatus(INT statusCode, LPTSTR http_status){
  LPCTSTR status;
  switch(statusCode){
    // Offical
    case 100:
      status = _T("Continue");
      break;
    case 101:
      status = _T("Switching Protocols");
      break;
    case 102:
      status = _T("Processing (WebDAV; RFC 2518)");
      break;
    case 200:
      status = _T("OK");
      break;
    case 201:
      status = _T("Created");
      break;
    case 202:
      status = _T("Accepted");
      break;
    case 203:
      status = _T("Non-Authoritative Information (since HTTP/1.1)");
      break;
    case 204:
      status = _T("No Content");
      break;
    case 205:
      status = _T("Reset Content");
      break;
    case 206:
      status = _T("Partial Content (RFC 7233)");
      break;
    case 207:
      status = _T("Multi-Status (WebDAV; RFC 4918)");
      break;
    case 208:
      status = _T("Already Reported (WebDAV; RFC 5842)");
      break;
    case 226:
      status = _T("IM Used (RFC 3229)");
      break;
    case 300:
      status = _T("Multiple Choices");
      break;
    case 301:
      status = _T("Moved Permanently");
      break;
    case 302:
      status = _T("Found");
      break;
    case 303:
      status = _T("See Other (since HTTP/1.1)");
      break;
    case 304:
      status = _T("Not Modified (RFC 7232)");
      break;
    case 305:
      status = _T("Use Proxy (since HTTP/1.1)");
      break;
    case 306:
      status = _T("Switch Proxy");
      break;
    case 307:
      status = _T("Temporary Redirect (since HTTP/1.1)");
      break;
    case 308:
      status = _T("Permanent Redirect (RFC 7538)");
      break;
    case 400:
      status = _T("Bad Request");
      break;
    case 401:
      status = _T("Unauthorized (RFC 7235)");
      break;
    case 402:
      status = _T("Payment Required");
      break;
    case 403:
      status = _T("Forbidden");
      break;
    case 404:
      status = _T("Not Found");
      break;
    case 405:
      status = _T("Method Not Allowed");
      break;
    case 406:
      status = _T("Not Acceptable");
      break;
    case 407:
      status = _T("Proxy Authentication Required (RFC 7235)");
      break;
    case 408:
      status = _T("Request Timeout");
      break;
    case 409:
      status = _T("Conflict");
      break;
    case 410:
      status = _T("Gone");
      break;
    case 411:
      status = _T("Length Required");
      break;
    case 412:
      status = _T("Precondition Failed (RFC 7232)");
      break;
    case 413:
      status = _T("Payload Too Large (RFC 7231)");
      break;
    case 414:
      status = _T("URI Too Long (RFC 7231)");
      break;
    case 415:
      status = _T("Unsupported Media Type");
      break;
    case 416:
      status = _T("Range Not Satisfiable (RFC 7233)");
      break;
    case 417:
      status = _T("Expectation Failed");
      break;
    case 418:
      status = _T("I'm a teapot (RFC 2324, RFC 7168)");
      break;
    case 421:
      status = _T("Misdirected Request (RFC 7540)");
      break;
    case 422:
      status = _T("Unprocessable Entity (WebDAV; RFC 4918)");
      break;
    case 423:
      status = _T("Locked (WebDAV; RFC 4918)");
      break;
    case 424:
      status = _T("Failed Dependency (WebDAV; RFC 4918)");
      break;
    case 426:
      status = _T("Upgrade Required");
      break;
    case 428:
      status = _T("Precondition Required (RFC 6585)");
      break;
    case 429:
      status = _T("Too Many Requests (RFC 6585)");
      break;
    case 431:
      status = _T("Request Header Fields Too Large (RFC 6585)");
      break;
    case 451:
      status = _T("Unavailable For Legal Reasons (RFC 7725)");
      break;
    case 500:
      status = _T("Internal Server Error");
      break;
    case 501:
      status = _T("Not Implemented");
      break;
    case 502:
      status = _T("Bad Gateway");
      break;
    case 503:
      status = _T("Service Unavailable");
      break;
    case 504:
      status = _T("Gateway Timeout");
      break;
    case 505:
      status = _T("HTTP Version Not Supported");
      break;
    case 506:
      status = _T("Variant Also Negotiates (RFC 2295)");
      break;
    case 507:
      status = _T("Insufficient Storage (WebDAV; RFC 4918)");
      break;
    case 508:
      status = _T("Loop Detected (WebDAV; RFC 5842)");
      break;
    case 510:
      status = _T("Not Extended (RFC 2774)");
      break;
    case 511:
      status = _T("Network Authentication Required (RFC 6585)");
      break;

    // Unoffical
    case 103:
      status = _T("Checkpoint");
      break;
    case 218:
      status = _T("This is fine (Apache Web Server)");
      break;
    case 420:
      status = _T("Method Failure (Spring Framework)");
      break;
    case 450:
      status = _T("Blocked by Windows Parental Controls (Microsoft)");
      break;
    case 498:
      status = _T("Invalid Token (Esri)");
      break;
    case 509:
      status = _T("Bandwidth Limit Exceeded (Apache Web Server/cPanel)");
      break;
    case 598:
      status = _T("(Informal convention) Network read timeout error");
      break;
    case 440:
      status = _T("Login Time-out");
      break;
    case 449:
      status = _T("Retry With");
      break;
    case 444:
      status = _T("No Response");
      break;
    case 494:
      status = _T("Request header too large");
      break;
    case 495:
      status = _T("SSL Certificate Error");
      break;
    case 496:
      status = _T("SSL Certificate Required");
      break;
    case 497:
      status = _T("HTTP Request Sent to HTTPS Port");
      break;
    case 499:
      status = _T("Client Closed Request");
      break;
    case 520:
      status = _T("Unknown Error");
      break;
    case 521:
      status = _T("Web Server Is Down");
      break;
    case 522:
      status = _T("Connection Timed Out");
      break;
    case 523:
      status = _T("Origin Is Unreachable");
      break;
    case 524:
      status = _T("A Timeout Occurred");
      break;
    case 525:
      status = _T("SSL Handshake Failed");
      break;
    case 526:
      status = _T("Invalid SSL Certificate");
      break;
    case 527:
      status = _T("Railgun Error");
      break;
    case 530:
      status = _T("Origin DNS Error");
      break;
    default:
      _stprintf_s(http_status, 100, _T("%s"), _T("Unknown"));
      return FALSE;
  }
  _stprintf_s(http_status, 100, _T("%s"), status);
  return TRUE;
}
