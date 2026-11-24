#pragma once
// Nova Studio 3.0: portable IDE project intelligence, minimap model and bounded AI patch format.
// No platform APIs, filesystem writes or network calls live here.
#include "studio_next14.hpp"
#include <algorithm>
#include <cwctype>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nova_next19 {
struct Symbol { std::wstring name; size_t line=0, offset=0; };
struct Hit { std::wstring path; size_t line=0; std::wstring preview; };
struct MiniLine { unsigned indent=0, extent=0; bool comment=false; };
inline std::wstring_view trim(std::wstring_view s){
  while(!s.empty()&&(s.front()==L' '||s.front()==L'\t'||s.front()==L'\r'))s.remove_prefix(1);
  while(!s.empty()&&(s.back()==L' '||s.back()==L'\t'||s.back()==L'\r'))s.remove_suffix(1);
  return s;
}
inline std::vector<Symbol> symbols(std::wstring_view source,size_t cap=256){
  std::vector<Symbol> out;
  size_t off=0,line=1;
  while(off<source.size()&&out.size()<cap){
    size_t end=source.find(L'\n',off);if(end==source.npos)end=source.size();
    auto text=source.substr(off,end-off);size_t indent=0;
    while(indent<text.size()&&(text[indent]==L' '||text[indent]==L'\t'))++indent;
    auto rest=text.substr(indent);
    constexpr std::wstring_view keys[]={L"function ",L"функція ",L"funkcja "};
    for(auto key:keys){
      if(rest.substr(0,key.size())!=key)continue;
      auto name=rest.substr(key.size());size_t n=0;
      while(n<name.size()&&nova_studio::wordChar(name[n]))++n;
      if(n && (n==name.size()||name[n]==L'('||name[n]==L' '||name[n]==L'\r'))
        out.push_back({std::wstring(name.substr(0,n)),line,off+indent+key.size()});
      break;
    }
    if(end==source.size())break;off=end+1;++line;
  }
  return out;
}
inline bool containsFolded(std::wstring_view line,std::wstring_view search){
  if(search.empty()||search.size()>line.size())return false;
  for(size_t i=0;i+search.size()<=line.size();++i){
    bool yes=true;for(size_t j=0;j<search.size();++j)
      if(std::towlower(line[i+j])!=std::towlower(search[j])){yes=false;break;}
    if(yes)return true;
  }
  return false;
}
inline std::vector<Hit> searchProject(const std::vector<std::pair<std::wstring,std::wstring>>& files,
                                      std::wstring_view query,size_t maxHits=150){
  std::vector<Hit> found;if(query.empty()||query.size()>128)return found;
  for(const auto& file:files){
    size_t start=0,line=1;
    while(start<file.second.size()&&found.size()<maxHits){
      auto end=file.second.find(L'\n',start);if(end==std::wstring::npos)end=file.second.size();
      auto text=trim(std::wstring_view(file.second).substr(start,end-start));
      if(containsFolded(text,query)){
        if(text.size()>135)text=text.substr(0,135);
        found.push_back({file.first,line,std::wstring(text)});
      }
      if(end==file.second.size())break;start=end+1;++line;
    }
    if(found.size()>=maxHits)break;
  }
  return found;
}
inline std::vector<MiniLine> miniLines(std::wstring_view text,size_t maxLines=12000){
  std::vector<MiniLine> lines;
  size_t pos=0;
  while(pos<=text.size()&&lines.size()<maxLines){
    size_t end=text.find(L'\n',pos);if(end==text.npos)end=text.size();
    auto line=text.substr(pos,end-pos);size_t first=0;
    while(first<line.size()&&(line[first]==L' '||line[first]==L'\t'))++first;
    const bool comment=first<line.size()&&line[first]==L'#';
    lines.push_back({(unsigned)std::min<size_t>(first,80),
      (unsigned)std::min<size_t>(line.size()-first,160),comment});
    if(end==text.size())break;pos=end+1;
  }
  return lines;
}
inline std::vector<std::wstring> smartSuggestions(std::wstring_view prefix,
    std::wstring_view doc, size_t limit=12){
  auto items=nova_studio::completions(prefix,limit);
  if(items.size()>=limit)return items;
  for(const auto& s:symbols(doc)){
    if(s.name.size()<prefix.size())continue;
    if(!containsFolded(std::wstring_view(s.name).substr(0,prefix.size()),prefix))continue;
    if(std::find(items.begin(),items.end(),s.name)==items.end())items.push_back(s.name);
    if(items.size()>=limit)break;
  }
  return items;
}
inline bool safeRelativeNova(std::wstring_view path){
  if(path.empty()||path.size()>180||path.front()==L'/'||path.front()==L'\\'||
     path.find(L':')!=path.npos||path.find(L'\\')!=path.npos||
     path.find(L'\0')!=path.npos||path.find(L'\r')!=path.npos)return false;
  if(path.size()<6||path.substr(path.size()-5)!=L".nova")return false;
  size_t pos=0;
  while(pos<path.size()){
    size_t end=path.find(L'/',pos);if(end==path.npos)end=path.size();
    auto seg=path.substr(pos,end-pos);
    if(seg.empty()||seg==L"."||seg==L".."||seg.back()==L'.'||seg.back()==L' '||
       seg.find(L'\n')!=seg.npos||seg.find(L'\t')!=seg.npos)return false;
    for(auto c:seg){if(c<32||c==L'<'||c==L'>'||c==L'"'||c==L'|'||c==L'?'||c==L'*')return false;}
    pos=end+1;
  }
  return true;
}
struct FilePatch {std::wstring relativePath, content;};
// Entire response must consist of explicit ```nova:relative/path.nova fenced blocks.
// This prevents prose masquerading as file edits and disallows arbitrary paths.
inline std::optional<std::vector<FilePatch>> parseMultiFile(std::wstring_view answer){
  std::vector<FilePatch> patches;std::set<std::wstring> unique;
  size_t pos=0;size_t budget=0;
  while(pos<answer.size()){
    constexpr std::wstring_view prefix=L"```nova:";
    if(answer.substr(pos,prefix.size())!=prefix)return std::nullopt;
    auto header=answer.find(L'\n',pos+prefix.size());if(header==answer.npos)return std::nullopt;
    auto path=answer.substr(pos+prefix.size(),header-pos-prefix.size());
    if(!path.empty()&&path.back()==L'\r')path.remove_suffix(1);
    if(!safeRelativeNova(path)||!unique.insert(std::wstring(path)).second)return std::nullopt;
    auto end=answer.find(L"\n```",header+1);if(end==answer.npos)return std::nullopt;
    auto source=answer.substr(header+1,end-(header+1));
    if(!source.empty()&&source.back()==L'\r')source.remove_suffix(1);
    if(source.empty()||source.size()>16000)return std::nullopt;
    budget+=source.size();if(budget>30000)return std::nullopt;
    patches.push_back({std::wstring(path),std::wstring(source)});
    if(patches.size()>6)return std::nullopt;
    pos=end+4;
    if(pos<answer.size()){
      if(answer[pos]==L'\r')++pos;
      if(pos==answer.size()||answer[pos]!=L'\n')return std::nullopt;
      ++pos;
    }
  }
  if(patches.empty())return std::nullopt;
  return patches;
}
inline std::wstring previewDiff(std::wstring_view before,std::wstring_view after,size_t maxLines=32){
  // Concise, deterministic line-level preview. Common prefix/suffix are not repeated.
  auto split=[](std::wstring_view text){
    std::vector<std::wstring_view> lines;
    size_t pos=0;
    while(pos<text.size()){
      auto end=text.find(L'\n',pos);if(end==text.npos)end=text.size();
      auto line=text.substr(pos,end-pos);
      if(!line.empty()&&line.back()==L'\r')line.remove_suffix(1);
      lines.push_back(line);if(end==text.size())break;pos=end+1;
    }
    return lines;
  };
  const auto a=split(before),b=split(after);
  size_t common=0;
  while(common<a.size()&&common<b.size()&&a[common]==b[common])++common;
  size_t tail=0;
  while(tail+common<a.size()&&tail+common<b.size()&&
      a[a.size()-1-tail]==b[b.size()-1-tail])++tail;
  std::wstring display=L"@@ around line "+std::to_wstring(common+1)+L" @@\n";
  size_t emitted=0;
  for(size_t i=common;i<a.size()-tail&&emitted<maxLines;++i,++emitted)
    display+=L"- "+std::wstring(a[i].substr(0,180))+L"\n";
  for(size_t i=common;i<b.size()-tail&&emitted<maxLines;++i,++emitted)
    display+=L"+ "+std::wstring(b[i].substr(0,180))+L"\n";
  if(a.size()-tail-common+b.size()-tail-common>emitted)display+=L"... (more changes; inspect before applying)\n";
  return display;
}

struct FileSnapshot {std::wstring path,original;};
inline std::optional<std::wstring> diffSummary(const std::vector<FilePatch>& changes,
  const std::vector<FileSnapshot>& originals){
  std::wstring result;size_t totalLines=0;
  for(const auto& c:changes){
    auto file=std::find_if(originals.begin(),originals.end(),[&](const FileSnapshot& f){return f.path==c.relativePath;});
    if(file==originals.end())return std::nullopt; // this version edits existing files only
    auto lines=[](std::wstring_view s){return (size_t)std::count(s.begin(),s.end(),L'\n')+1;};
    if(c.content==file->original)return std::nullopt;
    totalLines+=lines(c.content);
    if(totalLines>2000)return std::nullopt;
    result+=L"FILE: "+c.relativePath+L"\n  BEFORE: "+std::to_wstring(lines(file->original))+
      L" lines, "+std::to_wstring(file->original.size())+L" characters\n  AFTER: "+
      std::to_wstring(lines(c.content))+L" lines, "+std::to_wstring(c.content.size())+L" characters\n";
    // Show bounded changed-line context rather than dumping project sources to a modal.
    size_t start=0;while(start<file->original.size()&&start<c.content.size()&&
        file->original[start]==c.content[start])++start;
    result+=L"  First difference at character "+std::to_wstring(start+1)+L"\n";
    result+=previewDiff(file->original,c.content,24)+L"\n";
  }
  return result;
}
} // namespace nova_next19
