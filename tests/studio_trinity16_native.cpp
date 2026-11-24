#include "../studio/studio_trinity16.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace nova_trinity16;
 auto src=L"# покажи while, untouched\r\nlet n = true\r\nwhile n and not false\r\n    show \"show пока # no\" # pokaż\r\n    obj.show = 1\r\n    n = false\r\nend\r\n";
 auto uk=translate(src,Language::Ukrainian);
 assert(uk.find(L"нехай n = істина")!=std::wstring::npos);
 assert(uk.find(L"поки n і не хиба")!=std::wstring::npos);
 assert(uk.find(L"покажи \"show пока # no\" # pokaż")!=std::wstring::npos);
 assert(uk.find(L"obj.show = 1")!=std::wstring::npos);
 assert(uk.find(L"# покажи while, untouched")!=std::wstring::npos);
 assert(uk.find(L"\r\n")!=std::wstring::npos);
 auto pl=translate(uk,Language::Polish);
 assert(pl.find(L"dopóki n oraz nie fałsz")!=std::wstring::npos);
 assert(translate(pl,Language::English)==src);
 assert(translate(uk,Language::Ukrainian)==uk);
 assert(translate(L"show \"escaped \\\" if\"\n",Language::Polish)==L"pokaż \"escaped \\\" if\"\n");
 std::cout<<"Trinity16 source transform tests: OK\n";
}
