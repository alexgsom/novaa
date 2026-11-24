#include "../studio/studio_living17.hpp"
#include <cassert>
#include <iostream>
using namespace nova_designer15;
using namespace nova_living17;
int main(){
  Form f;f.title=L"План 'Żółw' \"Hi\"";
  f.add(Kind::Label);f.add(Kind::Input);f.add(Kind::Button);
  f.widgets[1].caption=L"Line1\nLine2 \\ path";
  const auto text=exportEditable(f);
  auto imported=importEditable(text);assert(imported);
  assert(imported->title==f.title);
  assert(imported->widgets[1].caption==f.widgets[1].caption);
  imported->move(0,70,80);
  assert(importEditable(exportEditable(*imported))->widgets[0].x==70);
  auto crlf=text;size_t pos=0;
  while((pos=crlf.find(L'\n',pos))!=std::wstring::npos){crlf.insert(pos,L"\r");pos+=2;}
  assert(importEditable(crlf));
  auto modified=text;modified+=L"\nshow \"user edit\"\n";
  assert(!importEditable(modified));
  auto forged=text;forged.replace(0,1,L"!");assert(!importEditable(forged));
  auto danger=text;danger.replace(danger.find(L"# FORM END"),10,L"# FORM BAD");
  assert(!importEditable(danger));
  assert(!importEditable(L"show 1\n"));
  std::cout<<"Living17 roundtrip and safety tests: OK\n";
}
