//
//  GameScene.h
//  TankMultiplayer
//
//  Created by WuHao on 14-5-13.
//
//

#ifndef __TankMultiplayer__GameScene__
#define __TankMultiplayer__GameScene__

#include "cocos2d.h"
#include "Level.h"
#include "bullet.h"

class GameScene : public cocos2d::ParallaxNode
{
public:
    static cocos2d::Scene* createScene();
//    static GameScene* create();
    
    CREATE_FUNC(GameScene);
    virtual bool init();
    
    bool onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event);
    void onTouchMoved(cocos2d::Touch *touch, cocos2d::Event *event);
    void onTouchEnded(cocos2d::Touch *touch, cocos2d::Event *event);
    
    
    
    void update(float dt);
    
    CC_SYNTHESIZE(Level*, _level, Level);
    CC_SYNTHESIZE(cocos2d::Layer*, _PlayerLayer, PlayerLayer);
    CC_SYNTHESIZE(cocos2d::Layer*, _bulletLayer, BulletLayer);
    CC_SYNTHESIZE(cocos2d::Point, _wind, Wind);
    CC_SYNTHESIZE(cocos2d::Point, _gravity, Gravity);
    void initExplosionMasks();
    void initTests();
    Bullet* addBullet(BulletTypes type, cocos2d::Point pos, cocos2d::Point vector);
    cocos2d::Point offset;
    
    virtual void draw(cocos2d::Renderer* renderer, const kmMat4 &transform, bool transformUpdated);
    void explode(Bullet* bullet);
protected:
    GameScene():_click(false),_steps(2){};
    bool _click;
    int _steps;
    cocos2d::Sprite* _ex;
    cocos2d::Sprite* _burn;

};

#endif /* defined(__TankMultiplayer__GameScene__) */