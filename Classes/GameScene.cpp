//
//  GameScene.cpp
//  TankMultiplayer
//
//  Created by WuHao on 14-5-13.
//
//

#include "GameScene.h"
#include "Helper.h"

USING_NS_CC;

Scene* GameScene::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = GameScene::create();
    
    // add layer as a child to scene
    scene->addChild(layer);
    
    // return the scene
    return scene;
}

bool GameScene::init()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto offset = Point(visibleSize/2);
    
    
    //load map
    auto lvl = Level::create("map.png");
    this->addChild(lvl, 1, Point(1, 1), offset);
    this->setLevel(lvl);
    
    
    
    //load background
    auto background = Sprite::create("bluryBack.png");
    this->addChild(background, 3, Point(0.5, 0.5), offset);
    BlendFunc background_blendfunc={
        GL_ONE_MINUS_DST_ALPHA,
        GL_ONE
    };
    background->setBlendFunc(background_blendfunc);
    

    //layer for players
    auto playerLayer = Layer::create();
    this->addChild(playerLayer, 3, Point(1, 1), offset);
    playerLayer->setContentSize(lvl->getRT()->getContentSize());
    this->setPlayerLayer(playerLayer);
    playerLayer->setAnchorPoint(Point::ANCHOR_MIDDLE);
    playerLayer->ignoreAnchorPointForPosition(false);
    
    
    
    //layer for bullets
    auto bulletLayer =Layer::create();
    bulletLayer->setContentSize(lvl->getRT()->getContentSize());
    this->setBulletLayer(bulletLayer);
    bulletLayer->setAnchorPoint(Point::ANCHOR_MIDDLE);
    bulletLayer->ignoreAnchorPointForPosition(false);
    this->addChild(bulletLayer, 4, Point(1, 1), offset);
    
    //layer for effects

    
    //register touches
    auto listener = EventListenerTouchOneByOne::create();
    listener->onTouchBegan = CC_CALLBACK_2(GameScene::onTouchBegan, this);
    listener->onTouchEnded = CC_CALLBACK_2(GameScene::onTouchEnded, this);
    listener->onTouchMoved = CC_CALLBACK_2(GameScene::onTouchMoved, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    //default wind
    this->setWind(Point::ZERO);
    //default gravity
    this->setGravity(Point(0,-0.1));
    this->scheduleUpdate();
    
    //init explosion masks
    this->initExplosionMasks();
    
    
    //init tests
    this->initTests();
    return true;
}
void GameScene::initTests()
{
    auto p = TestNode::create();
    p->setPosition(500,800);
    p->setLastPos(Point(500,800));
    getPlayerLayer()->addChild(p);
}
void GameScene::initExplosionMasks()
{
    _ex = Sprite::create("expMask.png");
    _burn = Sprite::create("expMask2.png");
    BlendFunc cut;
    cut ={
        GL_ZERO,
        GL_ONE_MINUS_SRC_ALPHA
    };
    
    BlendFunc keepAlpha;
    keepAlpha={
        GL_DST_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    };
    //_ex->setBlendFunc(cut);
    
    //_burn->setBlendFunc(keepAlpha);
    _ex->retain();
    _burn->retain();
    
    _burn->setScale(1.05);
    _ex->addChild(_burn);
    _burn->setPosition(Point(_ex->getContentSize()/2));
    _ex->setScaleX(1.7);
    _ex->setScaleY(1.4);
}
void GameScene::draw(cocos2d::Renderer *renderer, const kmMat4 &transform, bool transformUpdated)
{
    //log("in draw");
}


bool GameScene::onTouchBegan(Touch* touch, Event* event)
{
    _click = true;
    return true;
}
void GameScene::onTouchMoved(Touch* touch, Event* event)
{
    _click = false;
    setPosition(getPosition()+touch->getDelta());
}
void GameScene::onTouchEnded(Touch* touch, Event* event)
{
    if(_click)
    {
        Point test(getBulletLayer()->convertToNodeSpace(touch->getLocation()));
        addBullet(defaultB, test, Point(2,5));

    }
}

Bullet* GameScene::addBullet(BulletTypes type, cocos2d::Point pos, cocos2d::Point vector)
{
    auto b = Bullet::create(type, pos, vector);
    _bulletLayer->addChild(b);
    return b;
}
void GameScene::explode(Bullet *bullet)
{
    auto aabb2 = _bulletLayer->getBoundingBox();
    Point offset(aabb2.origin+getPosition());
    auto pos = bullet->getPosition();
    _ex->setPosition(pos+offset);
    //TODO: set _ex size according to bullet config
    _ex->ManualDraw();
    bullet->runAction(RemoveSelf::create());
    
    //check to see if player got caught in the blast
    for(Node* player : _PlayerLayer->getChildren())
    {
        TestNode *p = dynamic_cast<TestNode*>(player);
        if(p->getPosition().getDistance(pos)< bullet->getConfig()->expRadius + p->radius)
        {
            p->airborn = true;
            //TODO: player should take damage
        }
    }
}

void GameScene::update(float dt)
{
    //hack alert::switch GL target to the map
    //kmGLPushMatrix();
    _level->getRT()->onBegin();
    //now the target is the render texture, we can begin reading the bullet and player pixels
    
    auto aabb2 = _bulletLayer->getBoundingBox();
    //Point offset(aabb2.origin+getPosition());
    aabb2.origin = Point::ZERO;
    for(Node* bullet : _bulletLayer->getChildren())
    {
        Bullet *b = dynamic_cast<Bullet*>(bullet);
        auto pos = b->getPosition();
        bool coll = false;
        int bulletRadius =b->getConfig()->radius;
        
        //check bounding box to see if the bullet is in the world
        auto aabb1 = b->getBoundingBox();
        if(aabb2.intersectsRect(aabb1))
        {
            //move this
            b->setPosition(pos+(pos-b->getLastPos())+getGravity()+getWind());
            b->setLastPos(pos);
            
            
            //check if we are colliding with a player
            for(Node *player : _PlayerLayer->getChildren())
            {
                TestNode* p = dynamic_cast<TestNode*>(player);
                if(b->getPosition().getDistance(p->getPosition()) < bulletRadius + p->radius)
                {
                    explode(b);
                    coll = true;
                    log("collide with player");
                    break;
                }
            }
            //if we didnt collide with player, then we have to check for the terrain
            if(!coll)
            {
                Point pos(b->getPosition());
                //init a buffer size big enough to hold a square that can contain a circle with specified radius
                int bufferSize = pow(bulletRadius*2, 2);
                Color4B *buffer = (Color4B*)malloc(sizeof(Color4B)*bufferSize);
                glReadPixels(pos.x-bulletRadius, pos.y-bulletRadius, bulletRadius*2, bulletRadius*2, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
                for(int i = 0; i < bufferSize; i++)
                {
                    //for accuracy, we only want the pixels in the circle
                    if(buffer[i].a>0 && Helper::isInCircle(i, bulletRadius))
                    {
                        explode(b);
                        coll = true;
                        break;
                    }
                }
                free(buffer);
            }
            //debug set color
            if(coll)
                b->setColor(Color3B::BLUE);
            else
                b->setColor(Color3B::YELLOW);
        }
        //if its not contained in the map
        else
        {
            b->runAction(RemoveSelf::create());
        }
    }
    
    
    //Finished looping bullet, now lets loop player
    for(Node* player : _PlayerLayer->getChildren())
    {
        TestNode* p = dynamic_cast<TestNode*>(player);
        if(p->airborn)
        {
            //move this
            auto pos2 = p->getPosition();
            auto pos = pos2+(pos2-p->getLastPos())+getGravity()+getWind();
            p->setPosition(pos);
            p->setLastPos(pos2);

            int radius = p->radius;
            int bufferSize = pow(radius*2, 2);
            Color4B *buffer = (Color4B*)malloc(sizeof(Color4B)*bufferSize);
            glReadPixels(pos.x-radius, pos.y-radius, radius*2, radius*2, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
            for(int i = 0; i < bufferSize; i++)
            {
                if(buffer[i].a> 0 && Helper::isInCircle(i, radius))
                {
                    //TODO: need to fix position, so does not clip with terrain, and get angle
                    p->airborn = false;
                    p->setLastPos(p->getPosition());
                    break;
                }
            }
            free(buffer);
        }
        
    }
    _level->getRT()->onEnd();
    //kmGLPopMatrix();
    
    
//    _level->getRT()->begin();
//        for(Node* bullet : _bulletLayer->getChildren())
//        {
//            Bullet *b = dynamic_cast<Bullet*>(bullet);
//            auto pos = b->getPosition();
//            if(b->willExplode())
//            {
//                //log("%f, %f", b->getPosition().x, b->getPosition().y);
//                _ex->setPosition(pos);
//                _ex->visit();
//                b->runAction(RemoveSelf::create());
//                //check to see if any player got caught in the blast
//                for(Node* player : _PlayerLayer->getChildren())
//                {
//                    TestNode *p = dynamic_cast<TestNode*>(player);
//                    if(!p->airborn && p->getPosition().getDistance(pos)< b->getConfig()->expRadius)
//                    {
//                        p->airborn = true;
//                    }
//                }
//            }
//            else
//            {
//                b->setPosition(pos+(pos-b->getLastPos())+getGravity()+getWind());
//                b->setLastPos(pos);
//            }
//        }
//    _level->getRT()->end();
//    for(Node* player : _PlayerLayer->getChildren())
//    {
//        TestNode *p = dynamic_cast<TestNode*>(player);
//        if(p->airborn)
//        {
//            auto pos = p->getPosition();
//            p->setPosition(pos+(pos-p->getLastPos())+getGravity()+getWind());
//            p->setLastPos(pos);
//        }
//    }
}