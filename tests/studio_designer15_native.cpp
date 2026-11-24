#include "../studio/studio_designer15.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace nova_designer15;
int main(){
  Form f;f.title=L"Київ \"Łódź\"";
  f.add(Kind::Label);f.add(Kind::Input);f.add(Kind::Button);f.add(Kind::Checkbox);f.add(Kind::Textarea);
  f.widgets[2].caption=L"Привіт \"Nova\"";
  const auto code=f.toNova();
  assert(code.find(L"ui.window(\"Київ \\\"Łódź\\\"\"")!=std::wstring::npos);
  assert(code.find(L"function on_button_3()")!=std::wstring::npos);
  assert(code.find(L"ui.button(app, \"Привіт \\\"Nova\\\"\", on_button_3)")!=std::wstring::npos);
  assert(code.find(L"ui.checkbox(app,")!=std::wstring::npos);
  assert(code.find(L"ui.textarea(app,")!=std::wstring::npos);
  assert(code.find(L"ui.run(app)")!=std::wstring::npos);
  f.move(1,-100,9999);assert(f.widgets[1].x==0&&f.widgets[1].y==f.height-f.widgets[1].h);
  f.resize(1,5000,5000);assert(f.widgets[1].w==f.width&&f.widgets[1].h==f.height);
  f.widgets[0].caption=L"a\\b\nc";
  assert(f.toNova().find(L"a\\\\b\\nc")!=std::wstring::npos);
  f.erase(2);assert(f.widgets.size()==4);
  f.title=std::wstring(L"bad")+L'\0';
  bool blocked=false;try{(void)f.toNova();}catch(const std::invalid_argument&){blocked=true;}
  assert(blocked);
  Form many;for(int i=0;i<32;++i)many.add(Kind::Label);
  blocked=false;try{many.add(Kind::Label);}catch(const std::length_error&){blocked=true;}
  assert(blocked);
  std::cout<<"Nova designer model: OK\n";
}
