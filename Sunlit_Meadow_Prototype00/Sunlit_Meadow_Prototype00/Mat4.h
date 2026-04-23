#pragma once
#ifndef MAT4_H
#define MAT4_H

#include "Vectors.h"

typedef struct {
    float m[4][4];
} Mat4;

/* Returns the 4x4 identity matrix */
Mat4 mat4Identity(void);

/* Multiplies two 4x4 matrices: result = a * b */
Mat4 mat4Mul(Mat4 a, Mat4 b);

/*
 * Perspective projection matrix.
 * fovY   - vertical field of view in radians
 * aspect - width / height
 * near   - near clip plane
 * far    - far clip plane
 */
Mat4 mat4Perspective(float fovY, float aspect, float near, float far);

/* Translation matrix: moves by (x, y, z) */
Mat4 mat4Translate(float x, float y, float z);

/*
 * Rotation matrix from pitch, yaw, and roll in radians.
 * Pitch = rotation around X axis
 * Yaw   = rotation around Y axis
 * Roll  = rotation around Z axis
 */
Mat4 mat4Rotate(float pitch, float yaw, float roll);

/* View matrix from eye position, target position, and up vector */
Mat4 mat4LookAt(Vec3 eye, Vec3 target, Vec3 up);

#endif /* MAT4_H */