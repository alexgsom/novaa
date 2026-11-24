#include "../studio/studio_aiproof18.hpp"
#include <cassert>
#include <iostream>
using namespace nova_aiproof18;
int main(){
 const std::wstring original=L"let a = 2\nshow a\n";
 auto p=prepare(original,10,16,L"show a",L"show a + 1");
 assert(p&&p->candidateDocument==L"let a = 2\nshow a + 1\n");
 assert(stillApplies(*p,original));
 assert(!stillApplies(*p,L"let a = 3\nshow a\n"));
 assert(!prepare(original,10,16,L"show b",L"show 5"));
 assert(!prepare(original,1,1,L"",L"show 5"));
 assert(!prepare(original,10,16,L"show a",L""));
 assert(!prepare(original,10,16,L"show a",std::wstring_view(L"x\0y",3),100));
 assert(!prepare(original,10,16,L"show a",L"123456789123456",25));
 assert(summary(*p).find(L"Compile-only")!=std::wstring::npos);
 std::cout<<"AI Proof18 patch review tests: OK\n";
}
