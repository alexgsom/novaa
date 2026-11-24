// Real native C++ tests for the same helpers used in the Windows editor.
#include "../studio/studio_text.hpp"
#include <stdexcept>
#define CHECK(x) do { if(!(x)) throw std::runtime_error("FAILED: " #x); } while(false)
#include <iostream>
using namespace nova_studio;
int main(){
  auto a=findNext(L"one ONE one",L"ONE"); CHECK(a && a.begin==0);
  a=findNext(L"one ONE one",L"ONE",1,true);CHECK(a && a.begin==4);
  a=findPrevious(L"a b a b",L"a",5);CHECK(a && a.begin==4);
  a=findPrevious(L"a b a b",L"a",0);CHECK(a && a.begin==4);
  CHECK(!findNext(L"one only",L"on",0,false,true));
  CHECK(findNext(L"one only",L"one",1,false,true).begin==0);
  CHECK(findNext(L"Київ ŁÓDŹ",L"київ",0,false,true).begin==0);
  CHECK(findNext(L"ŁÓDŹ",L"łódź",0,false,true).begin==0);
  auto r=replaceAll(L"cat scatter cat",L"cat",L"кіт",false,true);
  CHECK(r.text==L"кіт scatter кіт" && r.count==2);
  CHECK(replaceAll(L"abc",L"",L"X").text==L"abc");
  try{replaceAll(L"a a a",L"a",L"abcdefgh",false,false,10);CHECK(false);}catch(const std::length_error&){}
  CHECK(lineColumn(L"abc\r\nżółw\nкрай",8)==std::make_pair(size_t(2),size_t(4)));
  auto words=tokenize(L"нехай x = 42 # comment\nshow \"x # y\"\nniech b = 'hi'");
  int kw=0,number=0,comment=0,string=0;
  for(auto t:words){if(t.kind==Kind::Keyword)++kw;if(t.kind==Kind::Number)++number;
    if(t.kind==Kind::Comment)++comment;
    if(t.kind==Kind::String)++string;}
  CHECK(kw==3&&number==1&&comment==1&&string==2);
  std::wcout<<L"nova studio text helpers: PASS\n";
}
