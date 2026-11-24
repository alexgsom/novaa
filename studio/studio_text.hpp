#pragma once
// Portable, allocation-bounded text helpers shared by the Win32 editor and native tests.
// No Python, Windows SDK, or third-party dependencies.
#include <algorithm>
#include <cwctype>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nova_studio {
struct Match { size_t begin=std::wstring::npos, end=std::wstring::npos;
  explicit operator bool() const {return begin!=std::wstring::npos;} };
enum class Kind { Keyword, String, Number, Comment };
struct Token { size_t begin, end; Kind kind; };

inline wchar_t lower(wchar_t c) {
  // The Windows UI is UTF-16. Also cover the Ukrainian/Polish command alphabets
  // even when the current Windows locale is not one of those languages.
  if(c>=L'A' && c<=L'Z') return c+32;
  if(c>=0x0410 && c<=0x042F) return c+32; // А..Я
  if(c>=0x0400 && c<=0x040F) return c+80; // Є, І, Ї, Ґ etc.
  switch(c) {
    case L'\u0104': return L'\u0105'; // Ą → ą
    case L'\u0106': return L'\u0107'; // Ć → ć
    case L'\u0118': return L'\u0119'; // Ę → ę
    case L'\u0141': return L'\u0142'; // Ł → ł
    case L'\u0143': return L'\u0144'; // Ń → ń
    case L'\u00D3': return L'\u00F3'; // Ó → ó
    case L'\u015A': return L'\u015B'; // Ś → ś
    case L'\u0179': return L'\u017A'; // Ź → ź
    case L'\u017B': return L'\u017C'; // Ż → ż
    default: return std::towlower(c);
  }
}
inline bool wordChar(wchar_t c) {
  return c==L'_' || (c>=L'0'&&c<=L'9') || (c>=L'A'&&c<=L'Z') ||
         (c>=L'a'&&c<=L'z') || (c>=0x0400&&c<=0x052F) ||
         (c>=0x0100&&c<=0x024F) || std::iswalnum(c);
}
inline bool equalAt(std::wstring_view text,std::wstring_view query,size_t pos,bool sensitive){
  if(query.empty() || pos>text.size() || query.size()>text.size()-pos)return false;
  for(size_t j=0;j<query.size();++j)if((sensitive?text[pos+j]:lower(text[pos+j])) != (sensitive?query[j]:lower(query[j])))return false;
  return true;
}
inline bool boundary(std::wstring_view text,size_t pos,size_t length){
  return (pos==0||!wordChar(text[pos-1])) &&
         (pos+length>=text.size()||!wordChar(text[pos+length]));
}
inline Match findNext(std::wstring_view text,std::wstring_view query,size_t start=0,
                      bool sensitive=false,bool whole=false,bool wrap=true){
  if(query.empty()||query.size()>text.size())return {};
  start=std::min(start,text.size());
  auto scan=[&](size_t begin,size_t endExclusive)->Match{
    size_t limit=std::min(endExclusive,text.size()-query.size()+1);
    for(size_t i=begin;i<limit;++i)
      if(equalAt(text,query,i,sensitive)&&(!whole||boundary(text,i,query.size())))return {i,i+query.size()};
    return {};
  };
  auto match=scan(start,text.size());
  return match || !wrap ? match : scan(0,start);
}
inline Match findPrevious(std::wstring_view text,std::wstring_view query,size_t before,
                          bool sensitive=false,bool whole=false,bool wrap=true){
  if(query.empty()||query.size()>text.size())return {};
  before=std::min(before,text.size());
  auto scan=[&](size_t begin,size_t endExclusive)->Match{
    size_t limit=std::min(endExclusive,text.size()-query.size()+1);
    for(size_t i=limit;i-->begin;)
      if(equalAt(text,query,i,sensitive)&&(!whole||boundary(text,i,query.size())))return {i,i+query.size()};
    return {};
  };
  auto match=scan(0,before);
  return match || !wrap ? match : scan(before,text.size());
}
struct ReplaceResult {std::wstring text;size_t count=0;};
inline ReplaceResult replaceAll(std::wstring_view text,std::wstring_view from,std::wstring_view to,
                               bool sensitive=false,bool whole=false,size_t maxChars=4*1024*1024){
  if(from.empty())return {std::wstring(text),0};
  std::wstring out;out.reserve(std::min(text.size(),maxChars));size_t pos=0,count=0;
  while(pos<text.size()){
    auto hit=findNext(text,from,pos,sensitive,whole,false);
    if(!hit){out.append(text.substr(pos));break;}
    out.append(text.substr(pos,hit.begin-pos));out.append(to);pos=hit.end;++count;
    if(out.size()>maxChars)throw std::length_error("Replacement exceeds maximum document size");
  }
  if(out.size()>maxChars)throw std::length_error("Replacement exceeds maximum document size");
  return {std::move(out),count};
}
inline std::pair<size_t,size_t> lineColumn(std::wstring_view text,size_t offset){
  offset=std::min(offset,text.size());size_t line=1,col=1;
  for(size_t i=0;i<offset;++i){
    if(text[i]==L'\r'){++line;col=1;if(i+1<offset&&text[i+1]==L'\n')++i;}
    else if(text[i]==L'\n'){++line;col=1;}
    else ++col;
  }
  return {line,col};
}
inline bool isKeyword(std::wstring_view word){
  constexpr std::wstring_view names[]={
    L"let",L"show",L"ask",L"if",L"else",L"end",L"repeat",L"for",L"in",L"function",L"return",L"import",L"as",
    L"try",L"catch",L"match",L"case",L"break",L"continue",L"assert",L"raise",L"type",L"while",L"and",L"or",L"not",
    L"true",L"false",L"null",L"нехай",L"покажи",L"запитай",L"якщо",L"інакше",L"кінець",L"повтори",
    L"для",L"в",L"функція",L"поверни",L"спробуй",L"злови",L"випадок",L"зупини",L"продовж",
    L"niech",L"pokaż",L"zapytaj",L"jeśli",L"inaczej",L"koniec",L"powtórz",L"dla",L"w",
    L"funkcja",L"zwróć",L"spróbuj",L"złap",L"przypadek"
  };
  return std::find(std::begin(names),std::end(names),word)!=std::end(names);
}
inline std::vector<Token> tokenize(std::wstring_view text,size_t maxTokens=10000){
  std::vector<Token> out;size_t i=0;
  auto push=[&](size_t b,size_t e,Kind k){if(out.size()<maxTokens)out.push_back({b,e,k});};
  while(i<text.size() && out.size()<maxTokens){
    wchar_t c=text[i];size_t b=i;
    if(c==L'#'){while(i<text.size()&&text[i]!=L'\r'&&text[i]!=L'\n')++i;push(b,i,Kind::Comment);}
    else if(c==L'"'||c==L'\''){
      wchar_t quote=c;++i;
      while(i<text.size()){
        if(text[i]==L'\\'&&i+1<text.size()){i+=2;continue;}
        if(text[i++]==quote)break;
      }
      push(b,i,Kind::String);
    }else if(c>=L'0'&&c<=L'9'){
      while(i<text.size()&&((text[i]>=L'0'&&text[i]<=L'9')||text[i]==L'.'))++i;
      push(b,i,Kind::Number);
    }else if(wordChar(c)){
      while(i<text.size()&&wordChar(text[i]))++i;
      if(isKeyword(text.substr(b,i-b)))push(b,i,Kind::Keyword);
    }else ++i;
  }
  return out;
}
} // namespace nova_studio
