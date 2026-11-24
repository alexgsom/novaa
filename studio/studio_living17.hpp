#pragma once
// Nova Living UI 17: editable, lossless designer sources with strict roundtrip.
// Code is only reimported if it exactly matches the generated form (normalizing
// editor CRLF). This prevents overwriting hand-written callbacks or extra code.
#include "studio_designer15.hpp"
#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace nova_living17 {
struct DesignerChange {
  std::uint64_t documentId=0;
  std::wstring originalSnapshot;
  std::wstring proposed;
};
inline std::wstring normalizeNewlines(std::wstring_view text){
  std::wstring result;result.reserve(text.size());
  for(size_t i=0;i<text.size();++i){
    if(text[i]==L'\r'&&i+1<text.size()&&text[i+1]==L'\n')continue;
    result+=text[i];
  }
  return result;
}
inline const wchar_t* kindToken(nova_designer15::Kind kind){return nova_designer15::kindName(kind);}
inline std::wstring exportEditable(const nova_designer15::Form& form){
  form.validate();
  std::wstring source=L"# NOVA-STUDIO-FORM/1\n# FORM TITLE "+nova_designer15::novaString(form.title)+L"\n";
  source+=L"# FORM SIZE "+std::to_wstring(form.width)+L" "+std::to_wstring(form.height)+L"\n";
  for(const auto& w:form.widgets){
    source+=L"# FORM ITEM "+std::wstring(kindToken(w.kind))+L" "+std::to_wstring(w.x)+L" "+
      std::to_wstring(w.y)+L" "+std::to_wstring(w.w)+L" "+std::to_wstring(w.h)+
      L" "+nova_designer15::novaString(w.caption)+L"\n";
  }
  source+=L"# FORM END\n";
  return source+form.toNova();
}
inline bool parseUnsigned(std::wstring_view line,size_t& pos,int& value){
  while(pos<line.size()&&line[pos]==L' ')++pos;
  if(pos==line.size()||line[pos]<L'0'||line[pos]>L'9')return false;
  std::uint64_t n=0;
  while(pos<line.size()&&line[pos]>=L'0'&&line[pos]<=L'9'){
    n=n*10+(std::uint64_t)(line[pos++]-L'0');if(n>1000000)return false;
  }
  value=(int)n;return true;
}
inline bool parseQuoted(std::wstring_view line,size_t& pos,std::wstring& decoded){
  while(pos<line.size()&&line[pos]==L' ')++pos;
  if(pos>=line.size()||line[pos++]!=L'"')return false;
  while(pos<line.size()){
    auto c=line[pos++];
    if(c==L'"')return pos==line.size();
    if(c==L'\\'){
      if(pos==line.size())return false;
      wchar_t next=line[pos++];
      if(next==L'n')c=L'\n';else if(next==L'r')c=L'\r';
      else if(next==L't')c=L'\t';else if(next==L'\\'||next==L'"')c=next;
      else return false;
    }
    decoded+=c;
    if(decoded.size()>1000)return false;
  }
  return false;
}
inline std::optional<nova_designer15::Kind> parseKind(std::wstring_view name){
  using nova_designer15::Kind;
  if(name==L"Label")return Kind::Label;
  if(name==L"Input")return Kind::Input;
  if(name==L"Button")return Kind::Button;
  if(name==L"Checkbox")return Kind::Checkbox;
  if(name==L"Textarea")return Kind::Textarea;
  return std::nullopt;
}
inline std::optional<nova_designer15::Form> importEditable(std::wstring_view input){
  const std::wstring source=normalizeNewlines(input);
  constexpr std::wstring_view start=L"# NOVA-STUDIO-FORM/1\n";
  if(source.compare(0,start.size(),start)!=0)return std::nullopt;
  nova_designer15::Form form;form.widgets.clear();
  size_t pos=start.size();bool title=false,size=false,end=false;
  for(int lines=0;lines<40&&pos<source.size();++lines){
    auto next=source.find(L'\n',pos);if(next==std::wstring::npos)return std::nullopt;
    std::wstring_view line(source.data()+pos,next-pos);pos=next+1;
    if(line==L"# FORM END"){end=true;break;}
    if(line.substr(0,13)==L"# FORM TITLE "){
      if(title)return std::nullopt;
      size_t p=13;form.title.clear();
      if(!parseQuoted(line,p,form.title))return std::nullopt;
      title=true;
    }else if(line.substr(0,12)==L"# FORM SIZE "){
      if(size)return std::nullopt;
      size_t p=12;
      if(!parseUnsigned(line,p,form.width)||!parseUnsigned(line,p,form.height)||p!=line.size())return std::nullopt;
      size=true;
    }else if(line.substr(0,12)==L"# FORM ITEM "){
      auto kindEnd=line.find(L' ',12);if(kindEnd==line.npos)return std::nullopt;
      auto kind=parseKind(line.substr(12,kindEnd-12));if(!kind)return std::nullopt;
      size_t p=kindEnd+1;nova_designer15::Widget widget{*kind,0,0,0,0,L""};
      if(!parseUnsigned(line,p,widget.x)||!parseUnsigned(line,p,widget.y)||
         !parseUnsigned(line,p,widget.w)||!parseUnsigned(line,p,widget.h)||
         !parseQuoted(line,p,widget.caption))return std::nullopt;
      form.widgets.push_back(std::move(widget));
    }else return std::nullopt;
  }
  if(!title||!size||!end)return std::nullopt;
  try{
    // Reject noncanonical or hand-edited source rather than destroying work.
    if(exportEditable(form)!=source)return std::nullopt;
  }catch(...){return std::nullopt;}
  return form;
}
} // namespace nova_living17
