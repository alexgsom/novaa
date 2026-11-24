#pragma once
// Windows-only HTTPS transport. The API key stays in-process and never enters source files.
#include <windows.h>
#include <winhttp.h>
#include <cctype>
#include <stdexcept>
#include <string>
#include "nova_ai.hpp"

namespace nova_ai {
struct WinHttpHandle {
  HINTERNET value=nullptr;
  explicit WinHttpHandle(HINTERNET h):value(h){}
  ~WinHttpHandle(){if(value)WinHttpCloseHandle(value);}
  WinHttpHandle(const WinHttpHandle&)=delete;
  WinHttpHandle& operator=(const WinHttpHandle&)=delete;
};
inline std::wstring getEnv(const wchar_t* name){
  DWORD required=GetEnvironmentVariableW(name,nullptr,0);
  if(required==0||required>4096)return L"";
  std::wstring result(required,L'\0');
  DWORD copied=GetEnvironmentVariableW(name,result.data(),required);
  if(copied==0||copied>=required)return L"";
  result.resize(copied);return result;
}
inline std::string chat(const std::wstring& apiKey,const std::string& model,const std::string& prompt){
  if(apiKey.empty()||apiKey.size()>2048)throw std::runtime_error("OPENAI_API_KEY is missing or too long");
  for(wchar_t c:apiKey){
    // No CR/LF or other header injection can enter the Authorization header.
    if(!((c>=L'A'&&c<=L'Z')||(c>=L'a'&&c<=L'z')||(c>=L'0'&&c<=L'9')||c==L'-'||c==L'_'||c==L'.'))
      throw std::runtime_error("OPENAI_API_KEY contains invalid characters");
  }
  const std::string body=makeRequest(model,prompt);
  WinHttpHandle session(WinHttpOpen(L"NovaStudioNext/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
  if(!session.value)throw std::runtime_error("WinHttpOpen failed");
  // Bound the connection latency; all requests are performed off the UI thread.
  WinHttpSetTimeouts(session.value,6000,6000,15000,25000);
  WinHttpHandle connection(WinHttpConnect(session.value,L"api.openai.com",INTERNET_DEFAULT_HTTPS_PORT,0));
  if(!connection.value)throw std::runtime_error("WinHttpConnect failed");
  WinHttpHandle request(WinHttpOpenRequest(connection.value,L"POST",L"/v1/responses",
                               nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
  if(!request.value)throw std::runtime_error("WinHttpOpenRequest failed");
  const std::wstring headers=L"Content-Type: application/json\r\nAuthorization: Bearer "+apiKey+L"\r\n";
  if(!WinHttpSendRequest(request.value,headers.c_str(),(DWORD)headers.size(),
                         (LPVOID)body.data(),(DWORD)body.size(),(DWORD)body.size(),0))
    throw std::runtime_error("Cannot send AI request (check internet connection)");
  if(!WinHttpReceiveResponse(request.value,nullptr))throw std::runtime_error("AI service did not respond");
  DWORD code=0,size=sizeof(code);
  if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX))
    throw std::runtime_error("Cannot read AI service status");
  std::string response;
  while(true){
    DWORD available=0;
    if(!WinHttpQueryDataAvailable(request.value,&available))throw std::runtime_error("AI download failed");
    if(!available)break;
    if(response.size()+available>512*1024)throw std::runtime_error("AI response exceeds 512 KiB limit");
    char buffer[8192];
    while(available){
      DWORD read=0;
      DWORD chunk=available>(DWORD)sizeof(buffer)?(DWORD)sizeof(buffer):available;
      if(!WinHttpReadData(request.value,buffer,chunk,&read))throw std::runtime_error("AI download interrupted");
      if(!read)throw std::runtime_error("AI connection closed during download");
      response.append(buffer,read);available-=read;
    }
  }
  if(code!=200){
    try{(void)extractOutput(response);}catch(const std::exception& e){
      throw std::runtime_error("AI HTTP "+std::to_string(code)+": "+std::string(e.what()));
    }
    throw std::runtime_error("AI request failed, HTTP "+std::to_string(code));
  }
  return extractOutput(response);
}
}
