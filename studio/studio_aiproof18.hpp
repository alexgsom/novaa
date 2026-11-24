#pragma once
// Nova AI Proof 18: reviewable, identity-bound patch proposals.
// Successful compilation is evidence of syntactic acceptance, not correctness.
// Never run untrusted generated application code while checking a proposal.
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace nova_aiproof18 {
struct Proposal {
  std::wstring originalDocument;
  std::wstring originalSelection;
  std::wstring candidateDocument;
  size_t begin=0,end=0;
};
inline std::optional<Proposal> prepare(std::wstring_view snapshot,
                                       size_t begin,size_t end,
                                       std::wstring_view selected,
                                       std::wstring_view replacement,
                                       size_t limit=4*1024*1024){
  if(end<=begin||end>snapshot.size()||snapshot.substr(begin,end-begin)!=selected||
     replacement.empty()||replacement.find(L'\0')!=std::wstring_view::npos||
     snapshot.size()>limit||replacement.size()>limit-(snapshot.size()-(end-begin)))
    return std::nullopt;
  Proposal p;p.originalDocument=snapshot;p.originalSelection=selected;
  p.begin=begin;p.end=end;
  p.candidateDocument=std::wstring(snapshot.substr(0,begin));
  p.candidateDocument.append(replacement);
  p.candidateDocument.append(snapshot.substr(end));
  return p;
}
inline bool stillApplies(const Proposal& proposal,std::wstring_view current){
  return current==proposal.originalDocument &&
    proposal.end<=current.size() &&
    current.substr(proposal.begin,proposal.end-proposal.begin)==proposal.originalSelection;
}
inline size_t countLines(std::wstring_view text){
  return text.empty()?0:(size_t)1+std::count(text.begin(),text.end(),L'\n');
}
inline std::wstring summary(const Proposal& p){
  auto before=countLines(p.originalDocument),after=countLines(p.candidateDocument);
  return L"Before: "+std::to_wstring(before)+L" lines; after: "+
      std::to_wstring(after)+L" lines. Replacement characters: "+
      std::to_wstring(p.candidateDocument.size()-(p.originalDocument.size()-p.originalSelection.size()))+
      L". Compile-only check; the resulting app will NOT be executed.";
}
} // namespace nova_aiproof18
