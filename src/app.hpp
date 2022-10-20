#pragma once


class App{
public:

    App(){};
    ~App(){};



    void update(){
        ImGui::Begin("Window1");
        ImGui::Text("Witam Pana");
        ImGui::End();

    }



};