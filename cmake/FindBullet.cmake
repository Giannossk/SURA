# cmake/FindBullet.cmake
#
# Locates the vendored Bullet physics engine in third_party/bullet3.

if(TARGET BulletDynamics AND TARGET BulletCollision AND TARGET LinearMath)
    set(BULLET_FOUND TRUE)
    set(Bullet_FOUND TRUE)
    set(BULLET_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")
    set(BULLET_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")

    set(BULLET_DYNAMICS_LIBRARY BulletDynamics)
    set(BULLET_COLLISION_LIBRARY BulletCollision)
    set(BULLET_MATH_LIBRARY LinearMath)
    set(BULLET_LIBRARIES BulletDynamics BulletCollision LinearMath)

    if(TARGET BulletSoftBody)
        set(BULLET_SOFTBODY_LIBRARY BulletSoftBody)
        list(APPEND BULLET_LIBRARIES BulletSoftBody)
    endif()

    if(NOT TARGET Bullet::Bullet)
        add_library(Bullet::Bullet INTERFACE IMPORTED)
        set_target_properties(Bullet::Bullet PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${BULLET_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${BULLET_LIBRARIES}"
        )
    endif()
else()
    include("${CMAKE_ROOT}/Modules/FindBullet.cmake")
endif()
