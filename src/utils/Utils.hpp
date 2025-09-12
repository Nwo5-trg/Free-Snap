#pragma once

namespace Utils {
    cocos2d::CCPoint getSnappedPos(GameObject* obj);
    cocos2d::CCPoint rotatePoint(cocos2d::CCPoint p, cocos2d::CCPoint pivot, float rad);
}