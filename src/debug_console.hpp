#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <ctime>

#include "window_object.hpp"


class DebugLogger : public WindowObject{

    bool auto_scroll;

public:
    enum class Priority {
        SUCCESS,
        INFO,
        WARNING,
        ERROR,
    };

private:
    struct Data {
        Priority priority;
        std::string time;
        std::string msg;
        Data(Priority p, std::string t, std::string m): priority(p), time(t), msg(m) {};
    };

    std::vector<Data> log;


    ImU32 GetPriorityColor(Priority prio){
        switch (prio) {
        case Priority::SUCCESS:  return IM_COL32(70,  255, 64,  255);
        case Priority::INFO:     return IM_COL32(255, 255, 255, 255);
        case Priority::WARNING:  return IM_COL32(255, 237, 74,  255);
        case Priority::ERROR:    return IM_COL32(255, 74,  74,  255);
        default:                 return IM_COL32(255, 255, 255, 255);
        }
    }


public:
    DebugLogger(std::string name): WindowObject(name){}


    void PushBack(Priority p, std::string msg) {

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        
        std::stringstream time_ss;
        time_ss << std::setw(5) << log.size() << std::put_time(&tm, " - %H:%M:%S");

        log.emplace_back(p, time_ss.str().c_str(), msg);
    }


    void Render() override {
        if (!show) return;

        if(ImGui::Begin(window_name.c_str(), &show)){

            if (ImGui::Button("Clear")) log.clear();
            ImGui::SameLine();
            ImGui::Checkbox("Auto Scroll", &auto_scroll);


            ImGui::PushItemWidth(ImGui::GetWindowContentRegionMax().x);


            ImGui::BeginChild("##LogField");    
            
            ImGuiListClipper clipper;
            clipper.Begin(log.size());

            
            
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {

                    ImGui::PushID(i);
                    ImGui::BeginTable("##LogEntry", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings);

                    ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(115, 115, 115, 255));
                            ImGui::Text(log[i].time.c_str());
                            ImGui::PopStyleColor();

                        ImGui::TableSetColumnIndex(1);

                            ImGui::PushStyleColor(ImGuiCol_Text, GetPriorityColor(log[i].priority));
                            ImGui::Text(log[i].msg.c_str());
                            ImGui::PopStyleColor();

                    ImGui::EndTable();
                    ImGui::PopID();

                }
                    
            }


            if (auto_scroll) ImGui::SetScrollHereY(0);

            ImGui::EndChild();

        }
        ImGui::End();
    }



};