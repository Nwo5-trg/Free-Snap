#include "Utils.hpp"
#include "../shared/Settings.hpp"
#include "../shared/Cache.hpp"

using namespace geode::prelude;

namespace Utils {
    CCPoint getSnappedPos(GameObject* obj) {
        auto offset = EditorUI::get()->offsetForKey(obj->m_objectID);
        // ifitworksitworksihonesltydontunderstanditbutidontcare
        auto pos = Cache::lastTouchPos - offset;
        float objSize = ObjectToolbox::sharedState()->gridNodeSizeForKey(obj->m_objectID);;
        float gridSize = Settings::gridSize * (objSize / 30.0f);

        return ccp(
            round(pos.x / gridSize) * gridSize,
            round(pos.y / gridSize) * gridSize
        ) + offset;
    }

    CCPoint rotatePoint(CCPoint p, CCPoint pivot, float rad) {
        p = p - pivot;
        float c = cosf(rad); 
        float s = sinf(rad);
        float t = p.x;
        p.x = t * c - p.y * s + pivot.x;
        p.y = t * s + p.y * c + pivot.y;
        return p;
    }
}