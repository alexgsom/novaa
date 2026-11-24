#pragma once
// Nova Trinity 16: source-preserving keyword views. No Win32 or network dependencies.
// A lexical transformation, NOT an AST rewrite: identifiers, comments, whitespace,
// quoted strings and member attributes are preserved. Keep language syntax additions
// in sync with Nova Native's ALIAS / LEX_ALIAS maps.
#include "studio_text.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nova_trinity16 {
enum class Language { English, Ukrainian, Polish };
struct Word { const wchar_t* en; const wchar_t* uk; const wchar_t* pl; bool leadingOnly; };
inline constexpr Word vocabulary[] = {
  {L"let",L"нехай",L"niech",true},
  {L"show",L"покажи",L"pokaż",true},
  {L"if",L"якщо",L"jeśli",true},
  {L"else",L"інакше",L"inaczej",true},
  {L"end",L"кінець",L"koniec",true},
  {L"repeat",L"повтори",L"powtórz",true},
  {L"while",L"поки",L"dopóki",true},
  {L"for",L"для",L"dla",true},
  {L"function",L"функція",L"funkcja",true},
  {L"return",L"поверни",L"zwróć",true},
  {L"break",L"перерви",L"przerwij",true},
  {L"continue",L"продовж",L"kontynuuj",true},
  {L"try",L"спробуй",L"spróbuj",true},
  {L"catch",L"злови",L"złap",true},
  {L"match",L"вибір",L"wybierz",true},
  {L"case",L"коли",L"przypadek",true},
  {L"assert",L"перевір",L"sprawdź",true},
  {L"raise",L"викинь",L"rzuć",true},
  {L"true",L"істина",L"prawda",false},
  {L"false",L"хиба",L"fałsz",false},
  {L"null",L"нічого",L"nic",false},
  {L"and",L"і",L"oraz",false},
  {L"or",L"або",L"lub",false},
  {L"not",L"не",L"nie",false},
};
inline const wchar_t* localized(const Word& w,Language target){
  return target==Language::Ukrainian?w.uk:target==Language::Polish?w.pl:w.en;
}
inline const Word* wordFor(std::wstring_view source){
  for(const auto& w:vocabulary)
    if(source==w.en||source==w.uk||source==w.pl)return &w;
  return nullptr;
}
inline std::wstring translate(std::wstring_view source,Language target){
  // Invariant: strings, comments, identifiers and line endings are byte-for-byte
  // unchanged. Only recognized standalone keywords are translated.
  std::wstring out;out.reserve(source.size()+64);
  bool lineStart=true,comment=false;wchar_t quote=0;bool escaped=false;
  wchar_t previousNonSpace=0;
  for(size_t i=0;i<source.size();){
    wchar_t ch=source[i];
    if(ch==L'\n'||ch==L'\r'){
      out+=ch;++i;lineStart=true;comment=false;previousNonSpace=0;
      continue;
    }
    if(comment){out+=ch;++i;continue;}
    if(quote){
      out+=ch;++i;
      if(escaped)escaped=false;
      else if(ch==L'\\')escaped=true;
      else if(ch==quote)quote=0;
      continue;
    }
    if(ch==L'#'){comment=true;out+=ch;++i;continue;}
    if(ch==L'\''||ch==L'"'){
      quote=ch;out+=ch;++i;lineStart=false;previousNonSpace=ch;continue;
    }
    if(nova_studio::wordChar(ch)&&ch!=L'_'&&!(ch>=L'0'&&ch<=L'9')){
      size_t end=i+1;
      while(end<source.size()&&nova_studio::wordChar(source[end]))++end;
      auto token=source.substr(i,end-i);
      const Word* word=wordFor(token);
      // Don't translate member properties: ui.show, obj.true, etc.
      // Statement keywords only at the start of the logical line.
      const bool eligible=word&&previousNonSpace!=L'.'&&
        (!word->leadingOnly||lineStart);
      if(eligible)out+=localized(*word,target);
      else out.append(token);
      lineStart=false;previousNonSpace=token.back();i=end;continue;
    }
    out+=ch;++i;
    if(ch!=L' '&&ch!=L'\t'){lineStart=false;previousNonSpace=ch;}
  }
  return out;
}
} // namespace nova_trinity16
