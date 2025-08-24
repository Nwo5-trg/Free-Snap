#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(EditUI, EditorUI) { // ok dont judge me too bad for my code half of it is from months ago
    struct Fields {
        bool doSnap;
        // robtops is 0 whenevr moving so
        CCPoint lastTouchPos;

        bool snapIndicator;
        bool selectChroma;
        bool snapChroma;
        float chromaSpeed;
        float snapIndicatorOutlineThickness;
        float gridSize;
        ccColor3B selectObjectColor;
        ccColor3B snapObjectColor;
        ccColor4F snapIndicatorFill;
        ccColor4F snapIndicatorOutline;

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

        fields->doSnap = false;

        auto mod = Mod::get();
        fields->snapIndicator = mod->getSettingValue<bool>("snap-indicator");
        fields->snapChroma = mod->getSettingValue<bool>("snap-chroma");
        fields->selectChroma = mod->getSettingValue<bool>("select-chroma");
        fields->chromaSpeed = mod->getSettingValue<double>("chroma-speed");
        fields->snapIndicatorOutlineThickness = mod->getSettingValue<double>("snap-indicator-outline-thickness");
        fields->gridSize = mod->getSettingValue<double>("grid-size");
        fields->selectObjectColor = mod->getSettingValue<ccColor3B>("select-object-color");
        fields->snapObjectColor = mod->getSettingValue<ccColor3B>("snap-object-color");
        fields->snapIndicatorFill = ccc4FFromccc4B(mod->getSettingValue<ccColor4B>("snap-indicator-fill"));
        fields->snapIndicatorOutline = ccc4FFromccc4B(mod->getSettingValue<ccColor4B>("snap-indicator-outline"));

        // idc if its like the same thing repeated twice i dont feel like refactoring its late
        auto snapChromaNode = CCNodeRGBA::create();
        snapChromaNode->runAction(CCRepeatForever::create(
            CCSequence::create(
                CCTintTo::create(fields->chromaSpeed, 255, 128, 128), 
                CCTintTo::create(fields->chromaSpeed, 255, 255, 128),
                CCTintTo::create(fields->chromaSpeed, 128, 255, 128), 
                CCTintTo::create(fields->chromaSpeed, 128, 255, 255),
                CCTintTo::create(fields->chromaSpeed, 128, 128, 255), 
                CCTintTo::create(fields->chromaSpeed, 255, 128, 255),
                nullptr
            )
        ));
        layer->addChild(snapChromaNode);
        fields->snapChromaNode = snapChromaNode;

        auto selectChromaNode = CCNodeRGBA::create();
        selectChromaNode->runAction(CCRepeatForever::create(
            CCSequence::create(
                CCTintTo::create(fields->chromaSpeed, 128, 255, 255),
                CCTintTo::create(fields->chromaSpeed, 128, 128, 255), 
                CCTintTo::create(fields->chromaSpeed, 255, 128, 255),
                CCTintTo::create(fields->chromaSpeed, 255, 128, 128), 
                CCTintTo::create(fields->chromaSpeed, 255, 255, 128),
                CCTintTo::create(fields->chromaSpeed, 128, 255, 128), 
                nullptr
            )
        ));
        layer->addChild(selectChromaNode);
        fields->selectChromaNode = selectChromaNode;

        CCDirector::sharedDirector()->getScheduler()->scheduleSelector(
            schedule_selector(EditUI::updateLoop), this, 0, false
        );

        return true;
    }

    void updateLoop(float dt) {
        auto fields = m_fields.self();
        tryMakeColorsGay();
        
        auto objs = getSelectedObjects();
        if (objs->count() == 0) return;

        for (auto obj : CCArrayExt<GameObject*>(objs)) {
            obj->setColor(fields->selectObjectColor);
        }

        // Dictionary | Definitions from Oxford Languages
        // par·a·noi·a | /ˌperəˈnoiə/
        // noun | noun: paranoia
        // 1. unjustified suspicion and mistrust of other people or their actions.
        // "I got into a state of paranoia about various night noises which in daylight seems utterly silly"
        // 2. the unwarranted or delusional belief that one is being 
        // persecuted, harassed, or betrayed by others, occurring as part of a mental condition.
        if (m_continueSwipe && m_snapObjectExists && m_snapObject) {
            updateSnapPreview(m_snapObject);
            m_snapObject->setColor(fields->snapObjectColor);
        }
    }

    bool ccTouchBegan(CCTouch* p0, CCEvent* p1) {
        if (!EditorUI::ccTouchBegan(p0, p1)) return false;
        auto gameManager = GameManager::sharedState();
        if (gameManager->getGameVariable("0008")) { // dumb workaround for disabling snap
            gameManager->setGameVariable("0008", false);
            m_fields->doSnap = true;
        }
        return true;
    }

    void ccTouchMoved(CCTouch* p0, CCEvent* p1) {
        m_fields->lastTouchPos = m_editorLayer->m_objectLayer->convertTouchToNodeSpace(p0);
        EditorUI::ccTouchMoved(p0, p1);
    }

    void ccTouchEnded(CCTouch* p0, CCEvent* p1) {
        auto fields = m_fields.self();

        bool continueSwipe = m_continueSwipe;
        // whaterver i said in `updateLoop()`
        auto obj = m_snapObjectExists ? m_snapObject : nullptr;

        EditorUI::ccTouchEnded(p0, p1);

        if (!fields->doSnap) return;
        GameManager::sharedState()->setGameVariable("0008", true);

        fields->snapDraw->clear();

        if (continueSwipe && obj) {
            auto snappedPos = getSnappedPos(obj);
            auto delta = snappedPos - obj->getPosition();

            auto objs = getSelectedObjects();
            for (auto obj : CCArrayExt<GameObject*>(objs)) {
                this->moveObject(obj, delta);
            }
            this->updateButtons();
            this->updateObjectInfoLabel();
            if (m_rotationControl->isVisible()) this->activateRotationControl(nullptr);
        }

        fields->doSnap = false;
    }

    void updateSnapPreview(GameObject* obj) {
        auto fields = m_fields.self();

        if (!fields->snapIndicator) return;

        auto pos = getSnappedPos(obj);
        auto scale = (obj->getScaledContentSize() - fields->snapIndicatorOutlineThickness / 2) / 2;
        float rotation = obj->getRotation();

        CCPoint v[] = {
            pos + scale, {pos.x - scale.width, pos.y + scale.height},
            pos - scale, {pos.x + scale.width, pos.y - scale.height}
        };

        if (rotation != 0.0f) {
            float rad = -rotation * 0.017453f;
            v[0] = rotatePoint(v[0], pos, rad);
            v[1] = rotatePoint(v[1], pos, rad);
            v[2] = rotatePoint(v[2], pos, rad);
            v[3] = rotatePoint(v[3], pos, rad);
        }

        fields->snapDraw->clear(); 
        fields->snapDraw->drawPolygon(
            v, 4, fields->snapIndicatorFill, 
            fields->snapIndicatorOutlineThickness, 
            fields->snapIndicatorOutline
        );
    }

    void tryMakeColorsGay() {
        auto fields = m_fields.self();

        if (fields->snapChroma) {
            auto chroma = ccc4FFromccc3B(
                // be gay do programming war crimes
                (fields->snapObjectColor = fields->snapChromaNode->getColor())
            );
            auto& fill = fields->snapIndicatorFill;
            fill.r = chroma.r; fill.g = chroma.g; fill.b = chroma.b;
            auto& outline = fields->snapIndicatorOutline;
            outline.r = chroma.r; outline.g = chroma.g; outline.b = chroma.b;
        }
    
        if (fields->selectChroma) fields->selectObjectColor = fields->selectChromaNode->getColor();
    }

    CCPoint getSnappedPos(GameObject* obj) {
        auto fields = m_fields.self();

        auto offset = offsetForKey(obj->m_objectID);
        // ifitworksitworksihonesltydontunderstanditbutidontcare
        auto pos = fields->lastTouchPos - offset;
        float objSize = ObjectToolbox::sharedState()->gridNodeSizeForKey(obj->m_objectID);;
        float gridSize = m_fields->gridSize * (objSize / 30.0f);

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
};