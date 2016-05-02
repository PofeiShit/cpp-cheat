/*
A sphere falling and hitting the ground.
*/

#include <cstdio>
#include <cstdlib>

#include <btBulletDynamicsCommon.h>

#define PRINTF_FLOAT "%7.3f"

constexpr float gravity = -10.0f;
constexpr float initialY = 10.0f;
constexpr float timeStep = 1.0f / 60.0f;
// TODO some combinations of coefficients smaller than 1.0
// make the ball go up higher / not lose height. Why?
constexpr float groundRestitution = 0.9f;
constexpr float sphereRestitution = 0.9f;
constexpr int maxNPoints = 500;

static void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    bool hadCollision = false;
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
        const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
        int numContacts = contactManifold->getNumContacts();
        for (int j = 0; j < numContacts; j++) {
            if (!hadCollision)
                std::printf("1 ");
            hadCollision = true;
            btManifoldPoint& pt = contactManifold->getContactPoint(j);
            if (pt.getDistance() < 0.0f) {
                const btVector3& ptA = pt.getPositionWorldOnA();
                const btVector3& ptB = pt.getPositionWorldOnB();
                const btVector3& normalOnB = pt.m_normalWorldOnB;
                std::printf(
                        PRINTF_FLOAT " " PRINTF_FLOAT " " PRINTF_FLOAT " "
                        PRINTF_FLOAT " " PRINTF_FLOAT " " PRINTF_FLOAT " "
                        PRINTF_FLOAT " " PRINTF_FLOAT " " PRINTF_FLOAT " ",
                        ptA.getX(), ptA.getY(), ptA.getZ(),
                        ptB.getX(), ptB.getY(), ptB.getZ(),
                        normalOnB.getX(), normalOnB.getY(), normalOnB.getZ());
            }
        }
    }
    if (!hadCollision)
        std::printf("0 ");
    puts("");
}

int main() {
    int i, j;

    btDefaultCollisionConfiguration *collisionConfiguration
            = new btDefaultCollisionConfiguration();
    btCollisionDispatcher *dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btBroadphaseInterface *overlappingPairCache = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    btDiscreteDynamicsWorld *dynamicsWorld = new btDiscreteDynamicsWorld(
            dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, gravity, 0));
    dynamicsWorld->setInternalTickCallback(myTickCallback);
    btAlignedObjectArray<btCollisionShape*> collisionShapes;

    // Ground.
    {
        btTransform groundTransform;

        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0, 0, 0));

        btCollisionShape* groundShape;
#if 1
        // x / z plane.
        // TODO: what is the planeConstant (second argument)?
        // Looks like the displacement from the origin.
        groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), -1);
#else
        // A cube of width 10 at y = -6.
        // Does not fall because we won't call:
        // colShape->calculateLocalInertia
        groundTransform.setOrigin(btVector3(0, -6, 0));
        groundShape = new btBoxShape(
                btVector3(btScalar(5.0), btScalar(5.0), btScalar(5.0)));

#endif
        collisionShapes.push_back(groundShape);
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0, myMotionState, groundShape, btVector3(0, 0, 0));
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setRestitution(groundRestitution);
		dynamicsWorld->addRigidBody(body);
    }

    // Sphere.
    {
        btCollisionShape* colShape = new btSphereShape(btScalar(1.0));
        collisionShapes.push_back(colShape);
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0, initialY, 0));
        btVector3 localInertia(0, 0, 0);
        btScalar mass(1.0f);
        colShape->calculateLocalInertia(mass, localInertia);
        btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody *body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
                mass, myMotionState, colShape, localInertia));
		body->setRestitution(sphereRestitution);
        dynamicsWorld->addRigidBody(body);
    }

    // Main loop.
    std::printf("step body x y z collision a b normal\n");
    for (i = 0; i < maxNPoints; ++i) {
        dynamicsWorld->stepSimulation(timeStep, 10);
        for (j = dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; --j) {
            btCollisionObject *obj = dynamicsWorld->getCollisionObjectArray()[j];
            btRigidBody *body = btRigidBody::upcast(obj);
            btTransform trans;
            if (body && body->getMotionState()) {
                body->getMotionState()->getWorldTransform(trans);
            } else {
                trans = obj->getWorldTransform();
            }
            btVector3 origin = trans.getOrigin();
            std::printf("%d %d " PRINTF_FLOAT " " PRINTF_FLOAT " " PRINTF_FLOAT " ",
                    i,
                    j,
                    float(origin.getX()),
                    float(origin.getY()),
                    float(origin.getZ()));
            int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
        }
    }

    // Cleanup.
    for (i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    for (i = 0; i < collisionShapes.size(); ++i) {
        delete collisionShapes[i];
    }
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
    collisionShapes.clear();
}
