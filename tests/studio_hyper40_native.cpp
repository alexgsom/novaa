// Cross-platform Hyper40 acceptance tests.
// Explicit checks are intentional: MSVC Release defines NDEBUG, and CRT assert
// failures may terminate with 0xC0000409 without a useful CTest message.
#include "../studio/studio_hyper40.hpp"
#include "../studio/studio_time_machine40.hpp"
#include "../studio/studio_forge40.hpp"
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs=std::filesystem;

// Never put a call that must execute inside an assert: Release builds differ.
#define CHECK(expr) do { \
  if(!(expr)) { \
    std::cerr << "[HYPER40 FAILED] " << __FILE__ << ':' << __LINE__ \
              << " : " #expr << std::endl; \
    return 1; \
  } \
} while(false)

static void progress(const char* section){
  std::cout << "[HYPER40] " << section << std::endl;
}

int main(){
 try{
  using namespace nova_hyper40;
  progress("Command Center / fuzzy ranking");
  CHECK(fuzzyScore(L"pqr",L"project quality runner")>=0);
  CHECK(fuzzyScore(L"ctrl",L"control")>=0);
  CHECK(fuzzyScore(L"nothing",L"good")<0);
  std::vector<Item> c={{L"Open project",L"Ctrl+O",L"",ItemType::Action,1,0},
      {L"Project X-Ray",L"Static analysis",L"",ItemType::Action,2,0},
      {L"app.nova",L"Project file",L"app.nova",ItemType::File,0,0}};
  auto xray=filter(L"x ray",c);
  CHECK(!xray.empty());
  CHECK(xray[0].index==1);
  auto files=filter(L"app",c);
  CHECK(files.size()==1);
  CHECK(files[0].index==2);

  progress("Project X-Ray / symbols and imports");
  auto report=inspect({{L"main.nova",L"import ui\nimport \"lib/helpers.nova\"\nfunction run()\nend\n# TODO improve\nfunction run()\nend\n"},
                       {L"lib/helpers.nova",L"show 42\n"}});
  CHECK(report.files==2);
  CHECK(report.functions==2);
  CHECK(report.todos==1);
  CHECK(report.imports==2);
  CHECK(report.issues.size()==2);
  CHECK(formatReport(report).find(L"PROJECT X-RAY")!=std::wstring::npos);
  CHECK(localImport(L"  import \"src/test.nova\" as module") ==L"src/test.nova");
  auto missing=inspect({{L"main.nova",L"import \"oops.nova\"\n"}});
  CHECK(missing.issues.size()==1);
  CHECK(missing.issues[0].warning);

  progress("Time Machine / filesystem and snapshots");
  // Unique per-run root: avoids collisions with another CI process or prior run.
  auto suffix=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  auto root=fs::temp_directory_path()/("nova40_native_test_"+suffix);
  std::error_code ec;
  bool created=fs::create_directories(root,ec);
  if(ec||!created){
    std::cerr << "[HYPER40 FAILED] Cannot create temporary directory: "
              << root.string() << " : " << ec.message() << std::endl;
    return 1;
  }
  // RAII cleanup also runs when a checked expectation fails via an exception.
  struct Cleanup { fs::path p; ~Cleanup(){std::error_code e;fs::remove_all(p,e);} } cleanup{root};
  auto source=root/"main.nova";
  {std::ofstream out(source,std::ios::binary);CHECK((bool)out);out<<"show 1\n";CHECK((bool)out);}
  CHECK(!nova_time40::relative(root,root.parent_path()/"unsafe.nova"));
  auto rel=nova_time40::relative(root,source);
  if(!rel)std::cerr << "[HYPER40] source relative path rejected: " << source.string() << std::endl;
  CHECK(rel.has_value());
  CHECK(*rel==L"main.nova");
  auto first=nova_time40::checkpoint(root,source,"show 1\n");
  if(!first)std::cerr << "[HYPER40] first checkpoint creation failed in " << root.string() << std::endl;
  CHECK(first.has_value());
  auto second=nova_time40::checkpoint(root,source,"show 2\n");
  if(!second)std::cerr << "[HYPER40] second checkpoint creation failed in " << root.string() << std::endl;
  CHECK(second.has_value());
  auto versions=nova_time40::list(root,source);
  if(versions.size()!=2){
    std::cerr << "[HYPER40] expected 2 checkpoints, found " << versions.size() << std::endl;
    for(const auto& p:versions)std::cerr << "  " << p.string() << std::endl;
  }
  CHECK(versions.size()==2);
  auto restored=nova_time40::read(root,source,*first);
  CHECK(restored.has_value());
  CHECK(*restored=="show 1\n");
  CHECK(nova_time40::read(root,source,root/"untrusted.nova")==std::nullopt);
  std::ifstream original(source,std::ios::binary);
  CHECK((bool)original);
  std::string stored((std::istreambuf_iterator<char>(original)),std::istreambuf_iterator<char>());
  CHECK(stored=="show 1\n"); // checkpoint must never overwrite source

  progress("Nova Forge / atomic scaffold and non-overwrite");
  auto project=root/"forge";
  CHECK(fs::create_directories(project));
  bool generated=nova_forge40::create(project,nova_forge40::Blueprint::Counter);
  if(!generated)std::cerr << "[HYPER40] Forge creation failed in " << project.string() << std::endl;
  CHECK(generated);
  CHECK(fs::exists(project/"main.nova"));
  CHECK(fs::exists(project/"README.md"));
  CHECK(!nova_forge40::create(project,nova_forge40::Blueprint::Tasks));
  CHECK(nova_forge40::make(nova_forge40::Blueprint::Notes).code.find("storage")!=std::string::npos);
  progress("PASS");
  return 0;
 }catch(const std::exception& e){
  std::cerr << "[HYPER40 EXCEPTION] " << e.what() << std::endl;
  return 1;
 }catch(...){
  std::cerr << "[HYPER40 EXCEPTION] unknown exception" << std::endl;
  return 1;
 }
}
