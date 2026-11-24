// Native HTTPS/HTTP requests. WinHTTP on Windows; libcurl on Linux/macOS.
// Certificate verification remains ON. No Python process involved.
#pragma once
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <winhttp.h>
#else
#include <curl/curl.h>
#endif
namespace nova_http {
constexpr size_t RESPONSE_LIMIT = 2 * 1024 * 1024;
static std::runtime_error error(const std::string& detail) {return std::runtime_error("http: "+detail);}
struct Result {int status=0;std::string body;};
static bool validUrl(const std::string& url){
  if(url.size()<8 || url.size()>4096)return false;
  bool scheme=url.rfind("http://",0)==0||url.rfind("https://",0)==0;
  if(!scheme)return false;
  for(unsigned char c:url)if(c<=32 || c==127)return false;
  return true;
}
#ifdef _WIN32
struct Handle {
  HINTERNET handle=nullptr;
  explicit Handle(HINTERNET h):handle(h){}
  ~Handle(){if(handle)WinHttpCloseHandle(handle);}
  explicit operator bool()const{return handle!=nullptr;}
};
static Result request(const std::string& method,const std::string& url,const std::string& payload="",const std::string& contentType="application/json"){
  if(!validUrl(url))throw error("URL must start with http:// or https:// and contain no whitespace");
  if(payload.size()>RESPONSE_LIMIT)throw error("request body exceeds 2 MiB");
  if(contentType.empty()||contentType.size()>150||contentType.find('\r')!=std::string::npos||contentType.find('\n')!=std::string::npos)throw error("invalid Content-Type");
  auto wurl=novaToWide(url);
  URL_COMPONENTSW p{};p.dwStructSize=sizeof(p);
  p.dwHostNameLength=(DWORD)-1;p.dwUrlPathLength=(DWORD)-1;p.dwExtraInfoLength=(DWORD)-1;
  if(!WinHttpCrackUrl(wurl.c_str(),(DWORD)wurl.size(),0,&p))throw error("invalid URL");
  if(p.dwHostNameLength==0)throw error("URL host is missing");
  std::wstring host(p.lpszHostName,p.dwHostNameLength);
  std::wstring route=p.dwUrlPathLength?std::wstring(p.lpszUrlPath,p.dwUrlPathLength):L"/";
  if(p.dwExtraInfoLength)route.append(p.lpszExtraInfo,p.dwExtraInfoLength);
  Handle session(WinHttpOpen(L"NovaNative/0.4",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
  if(!session)throw error("WinHttpOpen failed");
  WinHttpSetTimeouts(session.handle,5000,5000,10000,10000);
  Handle conn(WinHttpConnect(session.handle,host.c_str(),p.nPort,0));if(!conn)throw error("WinHttpConnect failed");
  bool tls=p.nScheme==INTERNET_SCHEME_HTTPS;
  Handle req(WinHttpOpenRequest(conn.handle,method=="POST"?L"POST":L"GET",route.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,tls?WINHTTP_FLAG_SECURE:0));
  if(!req)throw error("WinHttpOpenRequest failed");
  // Avoid unexpected redirect to a different protocol/host.
  DWORD noRedirect=WINHTTP_DISABLE_REDIRECTS;
  WinHttpSetOption(req.handle,WINHTTP_OPTION_DISABLE_FEATURE,&noRedirect,sizeof(noRedirect));
  auto header=novaToWide("Content-Type: "+contentType+"\r\n");
  if(!WinHttpSendRequest(req.handle,method=="POST"?header.c_str():WINHTTP_NO_ADDITIONAL_HEADERS,
                         method=="POST"?(DWORD)-1:0,
                         method=="POST"?(LPVOID)payload.data():WINHTTP_NO_REQUEST_DATA,
                         method=="POST"?(DWORD)payload.size():0,
                         method=="POST"?(DWORD)payload.size():0,0))throw error("WinHttpSendRequest failed");
  if(!WinHttpReceiveResponse(req.handle,nullptr))throw error("WinHttpReceiveResponse failed");
  DWORD code=0,size=sizeof(code);
  if(!WinHttpQueryHeaders(req.handle,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX))throw error("cannot read HTTP status");
  Result out;out.status=(int)code;
  char buffer[16384];DWORD read=0;
  while(true){if(!WinHttpReadData(req.handle,buffer,sizeof(buffer),&read))throw error("WinHttpReadData failed");
    if(!read)break;if(out.body.size()+read>RESPONSE_LIMIT)throw error("response body exceeds 2 MiB");out.body.append(buffer,read);
  }
  return out;
}
#else
struct CurlHandle {
  CURL* ptr;
  CurlHandle():ptr(curl_easy_init()) {if(!ptr)throw error("curl initialization failed");}
  ~CurlHandle(){curl_easy_cleanup(ptr);}
};
static size_t receive(char* data,size_t size,size_t nmemb,void* ptr){
  auto& out=*static_cast<std::string*>(ptr);
  if(size&&nmemb>RESPONSE_LIMIT/size)return 0;
  size_t count=size*nmemb;
  if(out.size()+count>RESPONSE_LIMIT)return 0;
  out.append(data,count);return count;
}
static Result request(const std::string& method,const std::string& url,const std::string& payload="",const std::string& contentType="application/json"){
  if(!validUrl(url))throw error("URL must start with http:// or https:// and contain no whitespace");
  if(payload.size()>RESPONSE_LIMIT)throw error("request body exceeds 2 MiB");
  static const auto setup=[](){return curl_global_init(CURL_GLOBAL_DEFAULT); }();
  if(setup!=CURLE_OK)throw error("curl initialization failed");
  CurlHandle curl;Result out;long code=0;
  curl_easy_setopt(curl.ptr,CURLOPT_URL,url.c_str());
  #if LIBCURL_VERSION_NUM >= 0x075500
  curl_easy_setopt(curl.ptr,CURLOPT_PROTOCOLS_STR,"http,https");
#else
  curl_easy_setopt(curl.ptr,CURLOPT_PROTOCOLS,CURLPROTO_HTTP|CURLPROTO_HTTPS);
#endif
  curl_easy_setopt(curl.ptr,CURLOPT_FOLLOWLOCATION,0L);
  curl_easy_setopt(curl.ptr,CURLOPT_CONNECTTIMEOUT_MS,5000L);
  curl_easy_setopt(curl.ptr,CURLOPT_TIMEOUT_MS,10000L);
  curl_easy_setopt(curl.ptr,CURLOPT_SSL_VERIFYPEER,1L);
  curl_easy_setopt(curl.ptr,CURLOPT_SSL_VERIFYHOST,2L);
  curl_easy_setopt(curl.ptr,CURLOPT_USERAGENT,"NovaNative/0.4");
  curl_easy_setopt(curl.ptr,CURLOPT_WRITEFUNCTION,receive);
  curl_easy_setopt(curl.ptr,CURLOPT_WRITEDATA,&out.body);
  struct curl_slist* headers=nullptr;
  if(method=="POST"){
    if(contentType.empty()||contentType.size()>150||contentType.find('\r')!=std::string::npos||contentType.find('\n')!=std::string::npos)throw error("invalid Content-Type");
    std::string header="Content-Type: "+contentType;
    headers=curl_slist_append(nullptr,header.c_str());
    curl_easy_setopt(curl.ptr,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(curl.ptr,CURLOPT_POST,1L);
    curl_easy_setopt(curl.ptr,CURLOPT_POSTFIELDS,payload.data());
    curl_easy_setopt(curl.ptr,CURLOPT_POSTFIELDSIZE_LARGE,(curl_off_t)payload.size());
  }
  auto rc=curl_easy_perform(curl.ptr);if(headers)curl_slist_free_all(headers);
  if(rc!=CURLE_OK)throw error(rc==CURLE_WRITE_ERROR?"response body exceeds 2 MiB":curl_easy_strerror(rc));
  curl_easy_getinfo(curl.ptr,CURLINFO_RESPONSE_CODE,&code);out.status=(int)code;
  return out;
}
#endif
}
