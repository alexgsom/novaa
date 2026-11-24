#include "../studio/studio_next14.hpp"
#include "../studio/nova_ai.hpp"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error("FAIL: " #x);}while(false)
int main(){
  auto candidates=nova_studio::completions(L"wh");
  CHECK(candidates.size()==1 && candidates[0]==L"while");
  CHECK(nova_studio::completions(L"пок").size()>=2);
  CHECK(nova_studio::completions(L"dop")[0]==L"dopóki");
  CHECK(nova_studio::completions(L"").empty());
  CHECK(nova_studio::completions(L"zzzz").empty());
  CHECK(nova_studio::completions(L"l",1).size()==1);
  CHECK(nova_studio::wordBefore(L"abc while",9)==std::make_pair(size_t(4),size_t(9)));
  CHECK(nova_studio::newlineIndent(L"if x",4)==L"\r\n    ");
  CHECK(nova_studio::newlineIndent(L"    while x",11)==L"\r\n        ");
  CHECK(nova_studio::newlineIndent(L"\tshow 2",7)==L"\r\n\t");
  CHECK(nova_studio::newlineIndent(L"поки x",6)==L"\r\n    ");
  CHECK(nova_studio::newlineIndent(L"dopóki x",8)==L"\r\n    ");
  CHECK(nova_studio::newlineIndent(L"  if x\r\n  show 2",17)==L"\r\n  ");
  auto errors=nova_studio::parseDiagnostics(L"[Compile]\r\nNova Native: Runtime error (line 3): missing variable\r\nwarning (line 8): unused\r\nDone");
  CHECK(errors.size()==2 && errors[0].line==3 && errors[1].line==8);
  CHECK(nova_studio::parseDiagnostics(L"All good\r\nshow 2").empty());
  CHECK(nova_studio::diagnosticLineNumber(L"Error at line 114: foo")==114);
  CHECK(nova_studio::diagnosticLineNumber(L"Error at line 0: foo")==0);
  auto code=nova_ai::extractNovaCode(L"This fixes it:\n```nova\nshow 42\n```\nExplanation");
  CHECK(code && *code==L"show 42");
  CHECK(!nova_ai::extractNovaCode(L"show 42"));
  CHECK(!nova_ai::extractNovaCode(L"```python\nprint(42)\n```"));
  CHECK(!nova_ai::extractNovaCode(L"```nova\nshow 2\n```\n```nova\nshow 3\n```"));
  CHECK(!nova_ai::extractNovaCode(L"```nova\n```"));
  std::cout<<"Nova Studio Next 14 native helpers: PASS\n";
}
