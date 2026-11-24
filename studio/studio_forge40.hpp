#pragma once
// Nova Forge: deterministic, offline project scaffolds; never replaces user files.
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>
namespace nova_forge40 {
namespace fs=std::filesystem;
enum class Blueprint { Counter=0, Notes=1, Tasks=2 };
struct Scaffold { std::string title, code, readme; };
inline Scaffold make(Blueprint kind){
  if(kind==Blueprint::Counter)return {"Counter",R"NV_0_END(# Mutable object shared by button event handlers
import ui
let app = ui.window("Nova Counter", 430, 280)
let state = {"count": 0}
let count_text = ui.label(app, "0")
function plus()
    state.count = state.count + 1
    count_text.set(str(state.count))
end
function reset()
    state.count = 0
    count_text.set("0")
end
let plus_button = ui.button(app, "+1", plus)
let reset_button = ui.button(app, "Reset", "reset")
ui.place(count_text, 24, 28, 340, 45)
ui.place(plus_button, 24, 95, 155, 40)
ui.place(reset_button, 205, 95, 155, 40)
ui.run(app)
)NV_0_END","# Nova Forge — Counter\n\nStarter Windows GUI with buttons and shared state.\n\nOpen `main.nova` in Nova Studio 4.0 and press **F6** to compile or **F5** to run.\nFor Windows GUI apps you can use **Build windowed EXE**.\nThe template uses only native Nova modules.\n"};
  else if(kind==Blueprint::Notes)return {"Notes",R"NV_1_END(# Note: files are written relative to the process working directory.
import ui
import storage
let app = ui.window("Nova Notes 0.9", 640, 520)
let notes = ui.textarea(app, "Привіт із Nova!")
function save()
    storage.write("nova_notes.txt", notes.get())
    ui.alert("Nova", "Нотатку збережено")
end
function load_notes()
    if storage.exists("nova_notes.txt")
        notes.set(storage.read("nova_notes.txt"))
    end
end
let save_button = ui.button(app, "Зберегти", save)
let open_button = ui.button(app, "Завантажити", load_notes)
ui.menu(app, "File", "Save", save)
ui.menu(app, "File", "Open", load_notes)
ui.place(notes, 20, 20, 580, 355)
ui.place(save_button, 20, 405, 200, 40)
ui.place(open_button, 245, 405, 200, 40)
ui.run(app)
)NV_1_END","# Nova Forge — Notes\n\nNative Notes app with persistent text storage.\n\nOpen `main.nova` in Nova Studio 4.0 and press **F6** to compile or **F5** to run.\nFor Windows GUI apps you can use **Build windowed EXE**.\nThe template uses only native Nova modules.\n"};
  else if(kind==Blueprint::Tasks)return {"Tasks",R"NV_2_END(# A real persistent command-line app: native storage + JSON + arguments.
# Works on Windows or Linux as a packaged standalone Nova program.
import system
import storage
import json
import lists

let folder = system.join(system.appdata(), "NovaTasks")
storage.mkdir(folder)
let datafile = system.join(folder, "tasks.json")
let tasks = []
if storage.exists(datafile)
    tasks = json.parse(storage.read(datafile))
end
let arguments = system.args()
if len(arguments) == 0
    show "Nova Tasks: add <text> | list | clear"
else
    if arguments[0] == "add"
        if len(arguments) < 2
            show "Please provide a task name"
        else
            lists.append(tasks, {"title":arguments[1],"done":false})
            storage.write(datafile, json.stringify(tasks))
            show "Added: " + arguments[1]
        end
    else
        if arguments[0] == "list"
            show "Nova Tasks"
            for task in tasks
                show "- " + task.title
            end
        else
            if arguments[0] == "clear"
                storage.write(datafile, "[]")
                show "Cleared"
            else
                show "Nova Tasks: add <text> | list | clear"
            end
        end
    end
end
)NV_2_END","# Nova Forge — Tasks\n\nOffline CLI task tracker with JSON persistence.\n\nOpen `main.nova` in Nova Studio 4.0 and press **F6** to compile or **F5** to run.\nFor Windows GUI apps you can use **Build windowed EXE**.\nThe template uses only native Nova modules.\n"};
  return {};
}
// The folder must already exist and be empty. No existing source is overwritten.
inline bool create(const fs::path& directory,Blueprint kind){
  try{
    if(!fs::is_directory(directory)||fs::is_symlink(directory))return false;
    if(fs::directory_iterator(directory)!=fs::directory_iterator())return false;
    auto blueprint=make(kind);
    if(blueprint.code.empty())return false;
    const auto source=directory/"main.nova";
    const auto doc=directory/"README.md";
    const auto scratch=directory/".nova-forge-partial";
    {std::ofstream out(scratch,std::ios::binary|std::ios::trunc);
     if(!out)return false;
     out<<blueprint.code;out.flush();if(!out){out.close();fs::remove(scratch);return false;}}
    try{fs::rename(scratch,source);}catch(...){fs::remove(scratch);return false;}
    {std::ofstream out(doc,std::ios::binary|std::ios::trunc);
     if(!out){fs::remove(source);return false;}
     out<<blueprint.readme;out.flush();
     if(!out){out.close();fs::remove(doc);fs::remove(source);return false;}}
    return true;
  }catch(...){return false;}
}
}
