#pragma once


#include <string>
#include <imgui.h>



class WindowObject{

protected:
    std::string window_name;
    bool show;

public:

    WindowObject(std::string name)
        : window_name(name), show (false) {}

    void Show(bool s){
        show = s;
    }

    bool IsShown() {
        return show;
    }

    virtual void Render() = 0;

};



