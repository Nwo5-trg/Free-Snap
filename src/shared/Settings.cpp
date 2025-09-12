#include "Settings.hpp"

using namespace geode::prelude;

namespace Settings {
    void updateSettings() {
        auto mod = Mod::get();
        
        snapIndicator = mod->getSettingValue<bool>("snap-indicator");
        snapChroma = mod->getSettingValue<bool>("snap-chroma");
        selectChroma = mod->getSettingValue<bool>("select-chroma");

        chromaSpeed = mod->getSettingValue<double>("chroma-speed");
        snapIndicatorOutlineThickness = mod->getSettingValue<double>("snap-indicator-outline-thickness");
        gridSize = mod->getSettingValue<double>("grid-size")
        ;
        selectObjectColor = mod->getSettingValue<ccColor3B>("select-object-color");
        snapObjectColor = mod->getSettingValue<ccColor3B>("snap-object-color");

        snapIndicatorFill = ccc4FFromccc4B(mod->getSettingValue<ccColor4B>("snap-indicator-fill"));
        snapIndicatorOutline = ccc4FFromccc4B(mod->getSettingValue<ccColor4B>("snap-indicator-outline"));
    }
}

$on_mod(Loaded) {
    Settings::updateSettings();

    listenForAllSettingChangesV3([] (std::shared_ptr<geode::SettingV3>) {
        Settings::updateSettings();
    });
}