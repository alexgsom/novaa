#pragma once
// Portable helpers for Nova Studio Next Preview 2: no Windows dependencies.
#include "studio_text.hpp"
#include <algorithm>
#include <cwctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nova_studio {
struct Diagnostic { size_t line=0;std::wstring message; };

// A suggestion is only offered after typing a prefix, avoiding huge unsolicited lists.
inline std::vector<std::wstring> completions(std::wstring_view prefix,size_t limit=12){
  static constexpr const wchar_t* words[]={
    L"show",L"let",L"ask",L"if",L"else",L"end",L"while",L"repeat",L"for",L"in",
    L"function",L"return",L"import",L"as",L"try",L"catch",L"match",L"case",
    L"break",L"continue",L"assert",L"raise",L"true",L"false",L"null",
    L"покажи",L"нехай",L"поки",L"якщо",L"інакше",L"кінець",L"повтори",
    L"для",L"функція",L"поверни",L"спробуй",L"злови",L"зупини",L"продовж",
    L"pokaż",L"niech",L"dopóki",L"jeśli",L"inaczej",L"koniec",L"powtórz",
    L"funkcja",L"zwróć",L"spróbuj",L"złap"
  };
  std::vector<std::wstring> result;
  if(prefix.empty() || prefix.size()>80 || !limit)return result;
  for(const auto* word:words){
    const std::wstring_view w(word);
    if(w.size()<prefix.size())continue;
    bool matches=true;
    for(size_t i=0;i<prefix.size();++i)
      if(lower(w[i])!=lower(prefix[i])){matches=false;break;}
    if(matches){result.emplace_back(word);if(result.size()>=limit)break;}
  }
  return result;
}
inline std::pair<size_t,size_t> wordBefore(std::wstring_view doc,size_t caret){
  caret=std::min(caret,doc.size());size_t first=caret;
  while(first>0&&wordChar(doc[first-1]))--first;
  return {first,caret};
}
inline std::wstring newlineIndent(std::wstring_view doc,size_t caret){
  caret=std::min(caret,doc.size());
  auto prev=caret?doc.rfind(L'\n',caret-1):std::wstring_view::npos;
  size_t begin=prev==std::wstring_view::npos?0:prev+1;
  size_t i=begin;
  while(i<caret&&(doc[i]==L' '||doc[i]==L'\t'))++i;
  std::wstring indent(doc.substr(begin,std::min<size_t>(i-begin,64)));
  // Only leading block keywords increase indentation; not a keyword inside a string.
  size_t start=i;
  while(i<caret&&wordChar(doc[i]))++i;
  const auto word=doc.substr(start,i-start);
  const bool opens=word==L"if"||word==L"while"||word==L"repeat"||word==L"for"||
    word==L"function"||word==L"try"||word==L"match"||
    word==L"якщо"||word==L"поки"||word==L"повтори"||word==L"для"||
    word==L"функція"||word==L"спробуй"||word==L"випадок"||
    word==L"jeśli"||word==L"dopóki"||word==L"powtórz"||word==L"funkcja"||
    word==L"spróbuj";
  if(opens&&i<caret&&indent.size()<=60)indent+=L"    ";
  return L"\r\n"+indent;
}
inline bool hasDiagnosticMark(std::wstring_view line){
  return line.find(L"error")!=line.npos||line.find(L"Error")!=line.npos||
         line.find(L"ERROR")!=line.npos||line.find(L"warning")!=line.npos||
         line.find(L"Warning")!=line.npos;
}
inline size_t diagnosticLineNumber(std::wstring_view line){
  // Nova's engine emits "(line N)"; the compiler also uses "line N".
  size_t pos=line.find(L"line ");
  if(pos==line.npos)pos=line.find(L"Line ");
  if(pos==line.npos)return 0;
  pos+=5;size_t value=0, count=0;
  while(pos<line.size()&&line[pos]>=L'0'&&line[pos]<=L'9'&&count<7){
    value=value*10+(size_t)(line[pos++]-L'0');++count;
  }
  if(!count||value==0||value>1000000)return 0;
  if(pos<line.size()&&line[pos]>=L'0'&&line[pos]<=L'9')return 0;
  return value;
}
inline std::vector<Diagnostic> parseDiagnostics(std::wstring_view text,size_t maxItems=64){
  std::vector<Diagnostic> items;
  for(size_t pos=0;pos<text.size()&&items.size()<maxItems;){
    size_t end=text.find(L'\n',pos);if(end==text.npos)end=text.size();
    auto line=text.substr(pos,end-pos);
    if(!line.empty()&&line.back()==L'\r')line.remove_suffix(1);
    if(line.size()>500)line=line.substr(0,500);
    if(hasDiagnosticMark(line)){
      size_t number=diagnosticLineNumber(line);
      if(number)items.push_back({number,std::wstring(line.substr(0,220))});
    }
    pos=end==text.size()?end:end+1;
  }
  return items;
}
}
