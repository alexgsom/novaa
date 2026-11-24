#include "../studio/studio_next19.hpp"
#include <cassert>
int main(){
  using namespace nova_next19;
  auto s=symbols(L"# function fake()\nshow \"function fake2()\"\nfunction start()\n end\nфункція привіт()\nfunkcja test()\n");
  assert(s.size()==3&&s[0].name==L"start"&&s[0].line==3&&s[1].name==L"привіт"&&s[2].name==L"test");
  auto hits=searchProject({{L"a.nova",L"Show hello\nother\nHELLO AGAIN"},{L"b.nova",L"hello"}},L"hello");
  assert(hits.size()==3&&hits[0].line==1&&hits[1].line==3);
  auto mini=miniLines(L"    # comment\n  show 123\n");
  assert(mini.size()==3&&mini[0].comment&&mini[1].indent==2);
  auto suggestions=smartSuggestions(L"tes",L"function tester()\nend\n");
  assert(!suggestions.empty()&&suggestions[0]==L"tester");
  for(auto unsafe:{L"../escape.nova",L"/absolute.nova",L"C:/bad.nova",L"a\\x.nova",L"foo.txt",L"a//b.nova",L"a/./b.nova"})assert(!safeRelativeNova(unsafe));
  assert(safeRelativeNova(L"src/main.nova"));
  auto p=parseMultiFile(L"```nova:a.nova\nshow 1\n```\n```nova:src/b.nova\nshow 2\n```");
  assert(p&&p->size()==2&&(*p)[1].content==L"show 2");
  assert(!parseMultiFile(L"explanation\n```nova:a.nova\nshow 1\n```"));
  assert(!parseMultiFile(L"```nova:a.nova\nshow 1\n```\n```nova:a.nova\nshow 2\n```"));
  assert(!parseMultiFile(L"```nova:../evil.nova\nshow 1\n```"));
  assert(!parseMultiFile(L"```nova:a.nova\nshow 1\n``` trailing text"));
  assert(diffSummary(*p,{{L"a.nova",L"show 3"},{L"src/b.nova",L"show 5"}}));
  assert(!diffSummary(*p,{{L"a.nova",L"show 3"}}));
}
