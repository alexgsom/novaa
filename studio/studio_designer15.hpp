#pragma once
// Nova Studio Next Preview 3: portable visual-form model and safe Nova code export.
// This file deliberately has no Win32 dependency so it can be tested on Linux and Windows.
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace nova_designer15 {
enum class Kind { Label, Input, Button, Checkbox, Textarea };
struct Widget {
  Kind kind;
  int x, y, w, h;
  std::wstring caption;
};
inline std::wstring novaString(const std::wstring& text){
  std::wstring result=L"\"";
  for(wchar_t c:text){
    if(c==L'\\')result+=L"\\\\";
    else if(c==L'"')result+=L"\\\"";
    else if(c==L'\n')result+=L"\\n";
    else if(c==L'\r')result+=L"\\r";
    else if(c==L'\t')result+=L"\\t";
    else if(c==0 || c<32)throw std::invalid_argument("Control character in form title");
    else result+=c;
  }
  return result+L'"';
}
inline const wchar_t* kindName(Kind k){
  switch(k){
    case Kind::Label:return L"Label";
    case Kind::Input:return L"Input";
    case Kind::Button:return L"Button";
    case Kind::Checkbox:return L"Checkbox";
    case Kind::Textarea:return L"Textarea";
  }
  return L"Unknown";
}
struct Form {
  std::wstring title=L"My Nova App";
  int width=780,height=520;
  std::vector<Widget> widgets;
  void add(Kind kind){
    if(widgets.size()>=32)throw std::length_error("32 widgets maximum");
    const int n=(int)widgets.size();
    int w=kind==Kind::Textarea?320:kind==Kind::Button?145:kind==Kind::Label?280:210;
    int h=kind==Kind::Textarea?105:kind==Kind::Button?40:32;
    widgets.push_back({kind,24+(n%4)*30,38+(n%5)*55,w,h,
       kind==Kind::Label?L"Welcome to Nova":kind==Kind::Input?L"Your name":
       kind==Kind::Button?L"Click me":kind==Kind::Checkbox?L"Enabled":L"Type here..."});
  }
  void move(size_t i,int x,int y){
    if(i>=widgets.size())throw std::out_of_range("Unknown widget");
    auto& v=widgets[i];v.x=std::clamp(x,0,std::max(0,width-v.w));
    v.y=std::clamp(y,0,std::max(0,height-v.h));
  }
  void resize(size_t i,int w,int h){
    if(i>=widgets.size())throw std::out_of_range("Unknown widget");
    auto& v=widgets[i];v.w=std::clamp(w,40,width);v.h=std::clamp(h,20,height);
    move(i,v.x,v.y);
  }
  void erase(size_t i){
    if(i>=widgets.size())throw std::out_of_range("Unknown widget");
    widgets.erase(widgets.begin()+(std::ptrdiff_t)i);
  }
  void validate()const{
    if(width<350||height<250||width>1500||height>1000||widgets.size()>32||title.size()>150)
      throw std::invalid_argument("Invalid form size or number of widgets");
    novaString(title);
    for(const auto& v:widgets){
      if(v.x<0||v.y<0||v.w<40||v.h<20||v.x+v.w>width||v.y+v.h>height||v.caption.size()>500)
        throw std::invalid_argument("Widget outside form or invalid size");
      novaString(v.caption);
    }
  }
  std::wstring toNova()const{
    validate();
    std::wstring result=L"# Generated with Nova Studio visual designer. Review code before running.\n"
      L"import ui\n\nlet app = ui.window("+novaString(title)+L", "+std::to_wstring(width)+
      L", "+std::to_wstring(height)+L")\n\n";
    // Buttons have callbacks defined before their use.
    for(size_t i=0;i<widgets.size();++i){
      if(widgets[i].kind!=Kind::Button)continue;
      const std::wstring name=L"on_button_"+std::to_wstring(i+1);
      result+=L"function "+name+L"()\n    ui.alert(\"Nova\", "+novaString(widgets[i].caption)+L")\nend\n\n";
    }
    for(size_t i=0;i<widgets.size();++i){
      const auto& v=widgets[i];
      const auto id=std::to_wstring(i+1);
      const std::wstring var=L"control_"+id;
      std::wstring expr;
      switch(v.kind){
        case Kind::Label:expr=L"ui.label(app, "+novaString(v.caption)+L")";break;
        case Kind::Input:expr=L"ui.input(app, "+novaString(v.caption)+L")";break;
        case Kind::Button:expr=L"ui.button(app, "+novaString(v.caption)+L", on_button_"+id+L")";break;
        case Kind::Checkbox:expr=L"ui.checkbox(app, "+novaString(v.caption)+L", false)";break;
        case Kind::Textarea:expr=L"ui.textarea(app, "+novaString(v.caption)+L")";break;
      }
      result+=L"let "+var+L" = "+expr+L"\n";
      result+=L"ui.place("+var+L", "+std::to_wstring(v.x)+L", "+std::to_wstring(v.y)+
        L", "+std::to_wstring(v.w)+L", "+std::to_wstring(v.h)+L")\n\n";
    }
    return result+L"ui.run(app)\n";
  }
};
} // namespace nova_designer15
