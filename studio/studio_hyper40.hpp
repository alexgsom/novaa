#pragma once
// Nova Studio 4: portable offline project intelligence + fuzzy command search.
// No Windows APIs, cloud calls, source execution or filesystem writes.
#include "studio_next19.hpp"
#include <algorithm>
#include <cwctype>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nova_hyper40 {
enum class ItemType { Action, File, Symbol };
struct Item { std::wstring label, detail, path; ItemType type=ItemType::Action; int action=0; size_t line=0; };
struct Scored { size_t index=0; int score=0; };
inline wchar_t lower(wchar_t c){return std::towlower(c);}
// Subsequence fuzzy matching: exact/prefix/word-boundary and adjacent matches rank highest.
inline int fuzzyScore(std::wstring_view query,std::wstring_view text){
  if(query.empty())return 1;
  if(query.size()>100||text.size()>1024)return -1;
  std::wstring needle;
  for(wchar_t c:query)if(c!=L' '&&c!=L'-'&&c!=L'_'&&c!=L'\t')needle+=c;
  if(needle.empty())return 1;
  int score=0,previous=-2;size_t j=0;
  for(size_t i=0;i<text.size()&&j<needle.size();++i){
    if(lower(text[i])!=lower(needle[j]))continue;
    score+=4;
    if(i==0)score+=12;
    if(i>0&&(text[i-1]==L'/'||text[i-1]==L'\\'||text[i-1]==L'_'||text[i-1]==L' '||text[i-1]==L'-'||text[i-1]==L'.'))score+=9;
    if(previous+1==(int)i)score+=8;
    previous=(int)i;++j;
  }
  if(j!=needle.size())return -1;
  if(needle.size()==text.size())score+=50;
  score-=static_cast<int>(std::min<size_t>(text.size(),500)/8);
  return score;
}
inline std::vector<Scored> filter(std::wstring_view query,const std::vector<Item>& items,size_t limit=35){
  std::vector<Scored> scores;
  for(size_t i=0;i<items.size();++i){
    int a=fuzzyScore(query,items[i].label);
    int b=fuzzyScore(query,items[i].detail);
    int c=items[i].path.empty()?-1:fuzzyScore(query,items[i].path);
    int s=std::max({a,b>0?b-3:b,c>0?c-2:c});
    if(s>=0)scores.push_back({i,s});
  }
  std::stable_sort(scores.begin(),scores.end(),[](const Scored& a,const Scored& b){return a.score>b.score;});
  if(scores.size()>limit)scores.resize(limit);
  return scores;
}
struct Source {std::wstring path, content;};
struct Issue {std::wstring path, message; size_t line=0; bool warning=false;};
struct Report {size_t files=0,lines=0,functions=0,imports=0,todos=0,bytes=0;std::vector<Issue> issues;};
inline bool isBuiltin(std::wstring_view module){
  return module==L"ui"||module==L"json"||module==L"math"||module==L"text"||
         module==L"lists"||module==L"storage"||module==L"sqlite"||
         module==L"system"||module==L"http";
}
inline std::wstring localImport(std::wstring_view text){
  text=nova_next19::trim(text);
  if(text.substr(0,7)!=L"import ")return {};
  text=nova_next19::trim(text.substr(7));
  if(text.empty())return {};
  if(text[0]==L'"'||text[0]==L'\''){
    auto end=text.find(text[0],1);if(end==text.npos)return {};
    return std::wstring(text.substr(1,end-1));
  }
  size_t end=0;while(end<text.size()&&nova_studio::wordChar(text[end]))++end;
  return std::wstring(text.substr(0,end));
}
inline Report inspect(const std::vector<Source>& sources,size_t maxIssues=250){
  Report out;std::set<std::wstring> known;
  for(const auto& f:sources){known.insert(f.path);}
  for(const auto& f:sources){
    if(f.content.size()>256*1024)continue;
    ++out.files;out.bytes+=f.content.size()*sizeof(wchar_t);
    std::set<std::wstring> foundFunctions;
    auto defs=nova_next19::symbols(f.content,1024);out.functions+=defs.size();
    for(const auto& def:defs){
      if(!foundFunctions.insert(def.name).second && out.issues.size()<maxIssues)
        out.issues.push_back({f.path,L"Duplicate function: "+def.name,def.line,true});
    }
    size_t pos=0,line=0;
    while(pos<=f.content.size()){
      ++line;++out.lines;
      const auto end=f.content.find(L'\n',pos);
      const auto last=end==std::wstring::npos?f.content.size():end;
      const auto raw=nova_next19::trim(std::wstring_view(f.content).substr(pos,last-pos));
      // TODOs in comments only, not string literals.
      const bool comment=!raw.empty()&&raw[0]==L'#';
      if(comment&&(nova_next19::containsFolded(raw,L"todo")||nova_next19::containsFolded(raw,L"fixme"))){
        ++out.todos;if(out.issues.size()<maxIssues)out.issues.push_back({f.path,L"TODO / FIXME marker",line,false});
      }
      if(!comment){
        auto mod=localImport(raw);
        if(!mod.empty()){
          ++out.imports;
          if(!isBuiltin(mod)){
            std::replace(mod.begin(),mod.end(),L'\\',L'/');
            std::wstring relative=mod;
            if(relative.size()<5||relative.substr(relative.size()-5)!=L".nova")relative+=L".nova";
            // Only assess explicit local .nova imports and single-segment module names.
            if(relative.find(L":")==std::wstring::npos&&!relative.empty()&&relative[0]!=L'/'&&
               relative.find(L"..") == std::wstring::npos){
              std::wstring base=f.path;
              auto slash=base.find_last_of(L'/');
              base=slash==std::wstring::npos?L"":base.substr(0,slash+1);
              if(!known.count(base+relative)&&!known.count(relative)&&out.issues.size()<maxIssues)
                out.issues.push_back({f.path,L"Unresolved local import: "+mod,line,true});
            }
          }
        }
      }
      if(end==std::wstring::npos)break;
      pos=end+1;
    }
  }
  return out;
}
inline std::wstring formatReport(const Report& r){
  std::wstring s=L"NOVA STUDIO 4  /  PROJECT X-RAY\r\n";
  s+=L"========================================\r\n\r\n";
  s+=L"FILES            "+std::to_wstring(r.files)+L"\r\n";
  s+=L"LINES            "+std::to_wstring(r.lines)+L"\r\n";
  s+=L"FUNCTIONS        "+std::to_wstring(r.functions)+L"\r\n";
  s+=L"IMPORTS          "+std::to_wstring(r.imports)+L"\r\n";
  s+=L"TODO / FIXME     "+std::to_wstring(r.todos)+L"\r\n";
  s+=L"FINDINGS         "+std::to_wstring(r.issues.size())+L"\r\n\r\n";
  for(const auto& issue:r.issues){
    s+=(issue.warning?L"[CHECK] ":L"[NOTE] ")+issue.path+L":"+
      std::to_wstring(issue.line)+L"  "+issue.message+L"\r\n";
  }
  if(r.issues.empty())s+=L"No static findings within the scanned files.\r\n";
  s+=L"\r\nOffline static scan only. Not a compiler or a security audit.\r\n";
  return s;
}
}
