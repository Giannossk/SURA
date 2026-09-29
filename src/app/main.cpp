// SURA -- application entry point.
//
// OGRE owns the window, the GPU device, the scene graph, materials and the
// frame loop. The physics layer is deliberately not wired up yet: the engine has
// not been chosen. The seam where it plugs in is marked below -- see AGENTS.md
// sections 2 and 5.

#include <Ogre.h>
#include <OgreApplicationContext.h>
#include <OgreCameraMan.h>
#include <OgreInput.h>
#include <OgreRTShaderSystem.h>
#include <OgreBullet.h>

#include <btBulletDynamicsCommon.h>

#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>
#include <vector>

namespace {

// Fixed simulation timestep. Physics solvers are tuned for a fixed dt and
// misbehave when fed variable frame time -- see AGENTS.md section 5.
constexpr float kSimulationDt = 1.0f / 90.0f;

class SuraApp : public OgreBites::ApplicationContext, public OgreBites::InputListener
{
public:
    SuraApp() : OgreBites::ApplicationContext("SURA") {}

    void setup() override
    {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);

        Ogre::Root* root = getRoot();
        mScnMgr = root->createSceneManager();

        // Register scene manager with RTShaderSystem
        auto* shadergen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
        if (shadergen)
        {
            shadergen->addSceneManager(mScnMgr);
        }

        // Camera & Viewport
        mCamera = mScnMgr->createCamera("MainCamera");
        mCamera->setNearClipDistance(0.05f);
        mCamera->setAutoAspectRatio(true);

        mCamNode = mScnMgr->getRootSceneNode()->createChildSceneNode("CameraNode");
        mCamNode->setPosition(0.0f, 0.8f, 2.5f);
        mCamNode->lookAt(Ogre::Vector3(0.0f, 0.1f, 0.0f), Ogre::Node::TS_PARENT);
        mCamNode->attachObject(mCamera);

        auto* vp = getRenderWindow()->addViewport(mCamera);
        vp->setBackgroundColour(Ogre::ColourValue(0.08f, 0.08f, 0.12f));

        // Light
        mScnMgr->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));
        Ogre::Light* dirLight = mScnMgr->createLight("MainDirectionalLight");
        dirLight->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* lightNode = mScnMgr->getRootSceneNode()->createChildSceneNode("LightNode");
        lightNode->setDirection(Ogre::Vector3(0.5f, -1.0f, -0.5f).normalisedCopy());
        lightNode->attachObject(dirLight);

        // Physics & Scene
        init_physics();
        create_scene();
    }

    bool keyPressed(const OgreBites::KeyboardEvent& evt) override
    {
        if (evt.keysym.sym == OgreBites::SDLK_ESCAPE)
        {
            getRoot()->queueEndRendering();
            return true;
        }

        if (evt.keysym.sym == OgreBites::SDLK_SPACE && mObjectBody)
        {
            // Reset the dynamic box position and velocities on Space key
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(0.0f, 1.5f, 0.0f));
            mObjectBody->setWorldTransform(transform);
            if (mObjectBody->getMotionState())
            {
                mObjectBody->getMotionState()->setWorldTransform(transform);
            }
            mObjectBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
            mObjectBody->setAngularVelocity(btVector3(1.2f, 2.5f, 0.8f));
            mObjectBody->clearForces();
            mObjectBody->activate(true);
            return true;
        }

        return OgreBites::InputListener::keyPressed(evt);
    }

    bool frameRenderingQueued(const Ogre::FrameEvent& evt) override
    {
        // =======================================================================
        // PHYSICS SEAM
        // -----------------------------------------------------------------------
        // Step the Bullet simulation world with a fixed timestep.
        // Ogre::Bullet::RigidBodyState automatically propagates the rigid body's
        // simulated transform directly to mObjectNode each step.
        // =======================================================================
        if (mDynamicsWorld)
        {
            mDynamicsWorld->stepSimulation(evt.timeSinceLastFrame, 10, kSimulationDt);
        }

        return true;
    }

    void shutdown() override
    {
        // Tear down physics world cleanly
        if (mDynamicsWorld)
        {
            for (auto& rb : mRigidBodies)
            {
                if (rb)
                {
                    mDynamicsWorld->removeRigidBody(rb.get());
                }
            }
            mRigidBodies.clear();
            mMotionStates.clear();
            mCollisionShapes.clear();
            mObjectBody = nullptr;
            mDynamicsWorld.reset();
            mSolver.reset();
            mBroadphase.reset();
            mDispatcher.reset();
            mCollisionConfig.reset();
        }

        if (mScnMgr)
        {
            auto* shadergen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
            if (shadergen)
            {
                shadergen->removeSceneManager(mScnMgr);
            }
            mScnMgr->clearScene();
            mScnMgr = nullptr;
        }
        OgreBites::ApplicationContext::shutdown();
    }

private:
    void init_physics()
    {
        mCollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
        mDispatcher      = std::make_unique<btCollisionDispatcher>(mCollisionConfig.get());
        mBroadphase      = std::make_unique<btDbvtBroadphase>();
        mSolver          = std::make_unique<btSequentialImpulseConstraintSolver>();
        mDynamicsWorld   = std::make_unique<btDiscreteDynamicsWorld>(
            mDispatcher.get(), mBroadphase.get(), mSolver.get(), mCollisionConfig.get());

        mDynamicsWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));

        // Static ground plane collider at y = -0.5
        auto groundShape = std::make_unique<btStaticPlaneShape>(btVector3(0.0f, 1.0f, 0.0f), -0.5f);
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0.0f, -0.5f, 0.0f));

        auto groundMotionState = std::make_unique<btDefaultMotionState>(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo groundRbInfo(0.0f, groundMotionState.get(), groundShape.get());
        auto groundBody = std::make_unique<btRigidBody>(groundRbInfo);
        groundBody->setRestitution(0.7f);
        groundBody->setFriction(0.6f);
        mDynamicsWorld->addRigidBody(groundBody.get());

        mCollisionShapes.push_back(std::move(groundShape));
        mMotionStates.push_back(std::move(groundMotionState));
        mRigidBodies.push_back(std::move(groundBody));
    }

    void create_scene()
    {
        // 1. Visual ground plane
        auto* groundManual = mScnMgr->createManualObject("GroundVisual");
        groundManual->begin("BaseWhite", Ogre::RenderOperation::OT_TRIANGLE_LIST);
        constexpr float gSize = 2.0f;
        constexpr float gY    = -0.5f;

        groundManual->position(-gSize, gY, -gSize);
        groundManual->normal(0.0f, 1.0f, 0.0f);
        groundManual->colour(0.2f, 0.25f, 0.35f, 1.0f);

        groundManual->position(gSize, gY, -gSize);
        groundManual->normal(0.0f, 1.0f, 0.0f);
        groundManual->colour(0.2f, 0.25f, 0.35f, 1.0f);

        groundManual->position(gSize, gY, gSize);
        groundManual->normal(0.0f, 1.0f, 0.0f);
        groundManual->colour(0.2f, 0.25f, 0.35f, 1.0f);

        groundManual->position(-gSize, gY, gSize);
        groundManual->normal(0.0f, 1.0f, 0.0f);
        groundManual->colour(0.2f, 0.25f, 0.35f, 1.0f);

        groundManual->triangle(0, 2, 1);
        groundManual->triangle(0, 3, 2);
        groundManual->end();

        auto* groundNode = mScnMgr->getRootSceneNode()->createChildSceneNode("GroundNode");
        groundNode->attachObject(groundManual);

        // 2. Simulated 3D Box
        constexpr float kHalf = 0.2f;
        create_box_mesh(kHalf);

        mObjectNode = mScnMgr->getRootSceneNode()->createChildSceneNode("SimulatedBoxNode");
        mObjectNode->setPosition(0.0f, 1.5f, 0.0f);
        mObjectNode->attachObject(mManualObject);

        // 3. Dynamic Bullet rigid body coupled via Ogre::Bullet::RigidBodyState
        auto boxShape = std::make_unique<btBoxShape>(btVector3(kHalf, kHalf, kHalf));
        btScalar mass = 1.0f;
        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        boxShape->calculateLocalInertia(mass, localInertia);

        auto objectMotionState = std::make_unique<Ogre::Bullet::RigidBodyState>(mObjectNode);
        btRigidBody::btRigidBodyConstructionInfo boxRbInfo(
            mass, objectMotionState.get(), boxShape.get(), localInertia);
        auto objectBody = std::make_unique<btRigidBody>(boxRbInfo);
        objectBody->setRestitution(0.65f);
        objectBody->setFriction(0.5f);
        objectBody->setAngularVelocity(btVector3(1.2f, 2.5f, 0.8f));

        mObjectBody = objectBody.get();
        mDynamicsWorld->addRigidBody(mObjectBody);

        mCollisionShapes.push_back(std::move(boxShape));
        mMotionStates.push_back(std::move(objectMotionState));
        mRigidBodies.push_back(std::move(objectBody));
    }

    void create_box_mesh(float h)
    {
        mManualObject = mScnMgr->createManualObject("SimulatedBox");
        mManualObject->setDynamic(true);
        mManualObject->begin("BaseWhite", Ogre::RenderOperation::OT_TRIANGLE_LIST);

        uint32_t baseIdx = 0;
        auto add_quad = [&](const Ogre::Vector3& p0, const Ogre::Vector3& p1,
                            const Ogre::Vector3& p2, const Ogre::Vector3& p3,
                            const Ogre::Vector3& normal, const Ogre::ColourValue& col)
        {
            mManualObject->position(p0); mManualObject->normal(normal); mManualObject->colour(col);
            mManualObject->position(p1); mManualObject->normal(normal); mManualObject->colour(col);
            mManualObject->position(p2); mManualObject->normal(normal); mManualObject->colour(col);
            mManualObject->position(p3); mManualObject->normal(normal); mManualObject->colour(col);

            mManualObject->triangle(baseIdx, baseIdx + 1, baseIdx + 2);
            mManualObject->triangle(baseIdx, baseIdx + 2, baseIdx + 3);
            baseIdx += 4;
        };

        // Front face (+Z)
        add_quad({-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}, { 0,  0,  1}, Ogre::ColourValue(0.9f, 0.3f, 0.2f, 1.0f));
        // Back face (-Z)
        add_quad({ h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h}, { 0,  0, -1}, Ogre::ColourValue(0.2f, 0.6f, 0.9f, 1.0f));
        // Top face (+Y)
        add_quad({-h,  h,  h}, { h,  h,  h}, { h,  h, -h}, {-h,  h, -h}, { 0,  1,  0}, Ogre::ColourValue(0.3f, 0.8f, 0.3f, 1.0f));
        // Bottom face (-Y)
        add_quad({-h, -h, -h}, { h, -h, -h}, { h, -h,  h}, {-h, -h,  h}, { 0, -1,  0}, Ogre::ColourValue(0.7f, 0.7f, 0.2f, 1.0f));
        // Right face (+X)
        add_quad({ h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h}, { 1,  0,  0}, Ogre::ColourValue(0.8f, 0.4f, 0.8f, 1.0f));
        // Left face (-X)
        add_quad({-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h}, {-1,  0,  0}, Ogre::ColourValue(0.2f, 0.8f, 0.8f, 1.0f));

        mManualObject->end();
    }

    Ogre::SceneManager* mScnMgr       = nullptr;
    Ogre::Camera*       mCamera       = nullptr;
    Ogre::SceneNode*    mCamNode      = nullptr;
    Ogre::ManualObject* mManualObject = nullptr;
    Ogre::SceneNode*    mObjectNode   = nullptr;

    // Bullet physics backend
    std::unique_ptr<btDefaultCollisionConfiguration>     mCollisionConfig;
    std::unique_ptr<btCollisionDispatcher>               mDispatcher;
    std::unique_ptr<btBroadphaseInterface>               mBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> mSolver;
    std::unique_ptr<btDiscreteDynamicsWorld>             mDynamicsWorld;

    std::vector<std::unique_ptr<btCollisionShape>>       mCollisionShapes;
    std::vector<std::unique_ptr<btMotionState>>          mMotionStates;
    std::vector<std::unique_ptr<btRigidBody>>            mRigidBodies;
    btRigidBody*                                         mObjectBody = nullptr;
};

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        SuraApp app;
        app.initApp();
        app.getRoot()->startRendering();
        app.closeApp();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "[OGRE] Exception caught: %s\n", e.what());
        return 1;
    }

    return 0;
}
