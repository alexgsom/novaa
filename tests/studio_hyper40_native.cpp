#include "../studio/studio_hyper40.hpp"
#include "../studio/studio_time_machine40.hpp"
#include "../studio/studio_forge40.hpp"
#undef NDEBUG
#include <cassert>
#include <filesystem>
#include <fstream>
int main(){
  using namespace nova_hyper40;
  assert(fuzzyScore(L"pqr",L"project quality runner")>=0);
  assert(fuzzyScore(L"ctrl",L"control")>=0);
  assert(fuzzyScore(L"nothing",L"good")<0);
  std::vector<Item> c={{L"Open project",L"Ctrl+O",L"",ItemType::Action,1,0},
      {L"Project X-Ray",L"Static analysis",L"",ItemType::Action,2,0},
      {L"app.nova",L"Project file",L"app.nova",ItemType::File,0,0}};
  assert(!filter(L"x ray",c).empty() && filter(L"x ray",c)[0].index==1);
  assert(filter(L"app",c).size()==1 && filter(L"app",c)[0].index==2);
  auto report=inspect({{L"main.nova",L"import ui\nimport \"lib/helpers.nova\"\nfunction run()\nend\n# TODO improve\nfunction run()\nend\n"},
                       {L"lib/helpers.nova",L"show 42\n"}});
  assert(report.files==2&&report.functions==2&&report.todos==1&&report.imports==2);
  assert(report.issues.size()==2);
  assert(formatReport(report).find(L"PROJECT X-RAY")!=std::wstring::npos);
  assert(localImport(L"  import \"src/test.nova\" as module") ==L"src/test.nova");
  auto missing=inspect({{L"main.nova",L"import \"oops.nova\"\n"}});
  assert(missing.issues.size()==1&&missing.issues[0].warning);
  namespace fs=std::filesystem;
  auto root=fs::temp_directory_path()/"nova40_native_test";
  fs::remove_all(root);fs::create_directories(root);
  auto source=root/"main.nova";
  {std::ofstream out(source);out<<"show 1\n";}
  assert(!nova_time40::relative(root,root.parent_path()/"unsafe.nova"));
  auto first=nova_time40::checkpoint(root,source,"show 1\n");assert(first);
  auto second=nova_time40::checkpoint(root,source,"show 2\n");assert(second);
  assert(nova_time40::list(root,source).size()==2);
  assert(nova_time40::read(root,source,*first)=="show 1\n");
  assert(nova_time40::read(root,source,root/"untrusted.nova")==std::nullopt);
  auto original=std::ifstream(source);std::string stored;std::getline(original,stored);
  assert(stored=="show 1"); // checkpoint never overwrites source
  auto project=root/"forge";
  fs::create_directories(project);
  assert(nova_forge40::create(project,nova_forge40::Blueprint::Counter));
  assert(fs::exists(project/"main.nova")&&fs::exists(project/"README.md"));
  assert(!nova_forge40::create(project,nova_forge40::Blueprint::Tasks)); // never overwrite
  assert(nova_forge40::make(nova_forge40::Blueprint::Notes).code.find("storage")!=std::string::npos);
  fs::remove_all(root);
}
