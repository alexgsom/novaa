#include "../studio/nova_ai.hpp"
#include <stdexcept>
#define CHECK(x) do { if(!(x)) throw std::runtime_error("AI test failed: " #x); } while(false)
#include <string>
int main(){
  using namespace nova_ai;
  CHECK(escapeJson("a\"\\\n\t") == "\"a\\\"\\\\\\n\\t\"");
  auto request=makeRequest("gpt-4.1-mini","Поясни: show \"ok\"");
  CHECK(request.find("\\\"ok\\\"")!=std::string::npos);
  CHECK(request.find("model")!=std::string::npos);
  CHECK(makeRequest("gpt-4.1-mini","Project request",8000).find("\"max_output_tokens\":8000")!=std::string::npos);
  CHECK(extractOutput("{\"output\":[{\"content\":[{\"text\":\"Привіт\\nNova\",\"type\":\"output_text\"}]}]}")=="Привіт\nNova");
  CHECK(extractOutput("{\"output\":[{\"content\":[{\"type\":\"output_text\",\"text\":\"\\u0141\\ud83d\\ude80\"}]}]}")=="Ł🚀");
  bool failed=false;
  try{extractOutput("{\"error\":{\"message\":\"invalid key\"}}");}catch(const std::runtime_error&){failed=true;}
  CHECK(failed);
  failed=false;
  try{extractOutput("{\"output\":[");}catch(const std::runtime_error&){failed=true;}
  CHECK(failed);
  return 0;
}
