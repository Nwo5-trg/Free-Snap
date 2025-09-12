#pragma once

namespace Settings {
    inline bool snapIndicator;
    inline bool selectChroma;
    inline bool snapChroma;
    
    inline float chromaSpeed;
    inline float snapIndicatorOutlineThickness;
    inline float gridSize;

    inline cocos2d::ccColor3B selectObjectColor;
    inline cocos2d::ccColor3B snapObjectColor;

    inline cocos2d::ccColor4F snapIndicatorFill;
    inline cocos2d::ccColor4F snapIndicatorOutline;

    void updateSettings();
}