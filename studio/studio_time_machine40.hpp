#pragma once
// Explicit local version checkpoints. Only files inside the chosen project are accepted.
// Snapshots preserve editor text but never overwrite source automatically.
#include "studio_next19.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
namespace nova_time40 {
namespace fs=std::filesystem;
inline std::optional<std::wstring> relative(const fs::path& root,const fs::path& source){
  try{
    if(!fs::is_directory(root)||!fs::is_regular_file(source)||fs::is_symlink(source))return {};
    const auto r=fs::weakly_canonical(root),s=fs::weakly_canonical(source);
    auto path=s.lexically_relative(r);
    if(path.empty()||path.is_absolute())return {};
    auto name=path.generic_wstring();
    if(!nova_next19::safeRelativeNova(name))return {};
    return name;
  }catch(...){return {};}
}
inline std::wstring key(std::wstring_view path){
  uint64_t hash=14695981039346656037ull;
  for(wchar_t c:path){hash^=(uint64_t)(uint32_t)c;hash*=1099511628211ull;}
  std::wostringstream out;out<<std::hex<<std::setfill(L'0')<<std::setw(16)<<hash;return out.str();
}
inline std::optional<fs::path> folder(const fs::path& root,const fs::path& source){
  auto rel=nova_time40::relative(root,source);if(!rel)return {};
  return root/L".nova-history"/key(*rel);
}
inline std::vector<fs::path> list(const fs::path& root,const fs::path& source){
  std::vector<fs::path> out;auto dir=folder(root,source);if(!dir)return out;
  std::error_code ec;
  if(!fs::is_directory(*dir,ec)||fs::is_symlink(*dir,ec)||fs::is_symlink(dir->parent_path(),ec))return out;
  for(const auto& item:fs::directory_iterator(*dir,fs::directory_options::skip_permission_denied,ec)){
    if(ec||out.size()>=100)break;
    if(item.is_regular_file(ec)&&!item.is_symlink(ec) &&
      item.path().extension()==L".nova" && item.path().filename().wstring().rfind(L"checkpoint_",0)==0 &&
      item.file_size(ec)<=4*1024*1024)out.push_back(item.path());
  }
  std::sort(out.begin(),out.end(),std::greater<fs::path>());return out;
}
inline std::optional<fs::path> checkpoint(const fs::path& root,const fs::path& source,const std::string& utf8){
  if(utf8.size()>4*1024*1024||utf8.find('\0')!=std::string::npos)return {};
  auto dir=folder(root,source);if(!dir)return {};
  try{
    // Check the history root before creating a child: never follow a redirected history directory.
    if(fs::is_symlink(dir->parent_path())||fs::is_symlink(*dir))return {};
    fs::create_directories(*dir);
    if(fs::is_symlink(*dir)||fs::is_symlink(dir->parent_path()))return {};
    auto now=std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    for(unsigned n=0;n<100;++n){
      const auto target=*dir/(L"checkpoint_"+std::to_wstring(now)+L"_"+std::to_wstring(n)+L".nova");
      if(fs::exists(target))continue;
      const auto staging=*dir/(L".partial_"+std::to_wstring(now)+L"_"+std::to_wstring(n));
      {std::ofstream out(staging,std::ios::binary|std::ios::trunc);
       if(!out)return {};
       out.write(utf8.data(),(std::streamsize)utf8.size());out.flush();
       if(!out){std::error_code ec;fs::remove(staging,ec);return {};}}
      try{fs::rename(staging,target);}catch(...){std::error_code ec;fs::remove(staging,ec);throw;}
      auto versions=list(root,source);
      for(size_t i=12;i<versions.size();++i){std::error_code ec;fs::remove(versions[i],ec);}
      return target;
    }
  }catch(...){return {};}
  return {};
}
inline std::optional<std::string> read(const fs::path& root,const fs::path& source,const fs::path& version){
  auto versions=list(root,source);
  if(std::find(versions.begin(),versions.end(),version)==versions.end())return {};
  try{
    std::ifstream in(version,std::ios::binary);if(!in)return {};
    std::string s((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    if(s.size()>4*1024*1024||s.find('\0')!=s.npos)return {};
    return s;
  }catch(...){return {};}
}
}
