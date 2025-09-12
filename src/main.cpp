#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "shared/Settings.hpp"
#include "shared/Cache.hpp"
#include "utils/Utils.hpp"

using namespace geode::prelude;

class $modify(EditUI, EditorUI) { // ok dont judge me too bad for my code half of it is from months ago
    struct Fields {
        CCLayer* snapLayer;
        CCDrawNode* snapDraw;
        CCNodeRGBA* snapChromaNode;
        CCNodeRGBA* selectChromaNode;
    };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto fields = m_fields.self();

        auto layer = CCLayer::create();
        layer->setPosition(0.0f, 0.0f);
        layer->setZOrder(-1500);
        layer->setID("free-snap-draw-layer"_spr);
        editorLayer->m_objectLayer->addChild(layer);
        fields->snapLayer = layer;

        auto draw = CCDrawNode::create();
        draw->setPosition(0.0f, 0.0f);
        draw->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        layer->addChild(draw);
        fields->snapDraw = draw;

        Cache::doSnap = false;

        // idc if its like the same thing repeated twice i dont feel like refactoring its late
        auto snapChromaNode = CCNodeRGBA::create();
        snapChromaNode->runAction(CCRepeatForever::create(
            CCSequence::create(
                CCTintTo::create(Settings::chromaSpeed, 255, 128, 128), 
                CCTintTo::create(Settings::chromaSpeed, 255, 255, 128),
                CCTintTo::create(Settings::chromaSpeed, 128, 255, 128), 
                CCTintTo::create(Settings::chromaSpeed, 128, 255, 255),
                CCTintTo::create(Settings::chromaSpeed, 128, 128, 255), 
                CCTintTo::create(Settings::chromaSpeed, 255, 128, 255),
                nullptr
            )
        ));
        layer->addChild(snapChromaNode);
        fields->snapChromaNode = snapChromaNode;

        auto selectChromaNode = CCNodeRGBA::create();
        selectChromaNode->runAction(CCRepeatForever::create(
            CCSequence::create(
                CCTintTo::create(Settings::chromaSpeed, 128, 255, 255),
                CCTintTo::create(Settings::chromaSpeed, 128, 128, 255), 
                CCTintTo::create(Settings::chromaSpeed, 255, 128, 255),
                CCTintTo::create(Settings::chromaSpeed, 255, 128, 128), 
                CCTintTo::create(Settings::chromaSpeed, 255, 255, 128),
                CCTintTo::create(Settings::chromaSpeed, 128, 255, 128), 
                nullptr
            )
        ));
        layer->addChild(selectChromaNode);
        fields->selectChromaNode = selectChromaNode;

        CCDirector::sharedDirector()->getScheduler()->scheduleSelector(
            schedule_selector(EditUI::updateSelect), this, 0, false
        );

        return true;
    }

    void updateSelect(float dt) {
        auto fields = m_fields.self();
        tryMakeColorsGay();
        
        auto objs = getSelectedObjects();
        if (objs->count() == 0) return;

        bool shouldntColorObjects = m_colorOverlay || m_hsvOverlay;

        // touches happen before schedulers so this works :3c
        for (auto obj : CCArrayExt<GameObject*>(objs)) {
            if (shouldntColorObjects) obj->updateObjectEditorColor();
            else obj->selectObject(Settings::selectObjectColor);
        }

        if (m_continueSwipe && m_snapObjectExists && Cache::doSnap) {
            updateSnapPreview(m_snapObject);
            if (shouldntColorObjects) m_snapObject->updateObjectEditorColor();
            else m_snapObject->selectObject(Settings::snapObjectColor);
        }
    }

    bool ccTouchBegan(CCTouch* p0, CCEvent* p1) {
        if (!EditorUI::ccTouchBegan(p0, p1)) return false;

        auto gameManager = GameManager::sharedState();
        if (gameManager->getGameVariable("0008")) { // dumb workaround for disabling snap
            gameManager->setGameVariable("0008", false);
            Cache::doSnap = true;
            Cache::lastTouchPos = m_editorLayer->m_objectLayer->convertTouchToNodeSpace(p0);
        }

        return true;
    }

    void ccTouchMoved(CCTouch* p0, CCEvent* p1) {
        Cache::lastTouchPos = m_editorLayer->m_objectLayer->convertTouchToNodeSpace(p0);
        EditorUI::ccTouchMoved(p0, p1);
    }

    void ccTouchEnded(CCTouch* p0, CCEvent* p1) {
        if (!Cache::doSnap) {
            EditorUI::ccTouchEnded(p0, p1);
            return;
        }

        bool continueSwipe = m_continueSwipe;
        auto obj = m_snapObjectExists ? m_snapObject : nullptr;

        EditorUI::ccTouchEnded(p0, p1);

        GameManager::sharedState()->setGameVariable("0008", true);
        m_fields->snapDraw->clear();

        if (continueSwipe && obj) {
            auto snappedPos = Utils::getSnappedPos(obj);
            auto delta = snappedPos - obj->getPosition();

            auto objs = getSelectedObjects();
            for (auto obj : CCArrayExt<GameObject*>(objs)) {
                this->moveObject(obj, delta);
            }

            this->updateButtons();
            this->updateObjectInfoLabel();
            if (m_rotationControl->isVisible()) {
                this->activateRotationControl(nullptr);
            }
        }

        Cache::doSnap = false;
    }

    void updateSnapPreview(GameObject* obj) {
        auto fields = m_fields.self();

        if (!Settings::snapIndicator) return;

        auto pos = Utils::getSnappedPos(obj);
        auto scale = (obj->getScaledContentSize() - Settings::snapIndicatorOutlineThickness / 2) / 2;
        float rotation = obj->getRotation();

        CCPoint v[] = {
            pos + scale, {pos.x - scale.width, pos.y + scale.height},
            pos - scale, {pos.x + scale.width, pos.y - scale.height}
        };

        if (rotation != 0.0f) {
            float rad = -rotation * 0.017453f;
            v[0] = Utils::rotatePoint(v[0], pos, rad);
            v[1] = Utils::rotatePoint(v[1], pos, rad);
            v[2] = Utils::rotatePoint(v[2], pos, rad);
            v[3] = Utils::rotatePoint(v[3], pos, rad);
        }

        fields->snapDraw->clear(); 
        fields->snapDraw->drawPolygon(
            v, 4, Settings::snapIndicatorFill, 
            Settings::snapIndicatorOutlineThickness, 
            Settings::snapIndicatorOutline
        );
    }

    void tryMakeColorsGay() {
        auto fields = m_fields.self();

        if (Settings::snapChroma) {
            auto chroma = ccc4FFromccc3B(
                // be gay do programming war crimes
                (Settings::snapObjectColor = fields->snapChromaNode->getColor())
            );
            auto& fill = Settings::snapIndicatorFill;
            fill.r = chroma.r; fill.g = chroma.g; fill.b = chroma.b;
            auto& outline = Settings::snapIndicatorOutline;
            outline.r = chroma.r; outline.g = chroma.g; outline.b = chroma.b;
        }
    
        if (Settings::selectChroma) {
            Settings::selectObjectColor = fields->selectChromaNode->getColor();
        }
    }
};