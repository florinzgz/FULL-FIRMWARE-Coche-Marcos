#ifndef HUDMANAGER_H
#define HUDMANAGER_H

#include "core/Manager.h"
#include "HUD.h"

class HUDManager : public Manager {
public:
    HUDManager() {}
    
    bool init() override {
        return true;
    }
    
    void update() override {
        HUD::update();
    }
};

#endif
