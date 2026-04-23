#include "Mat4.h"
#include "Vectors.h"
#include <math.h>

/* Returns the 4x4 identity matrix */
Mat4 mat4Identity(void) {
    Mat4 r = { 0 };
    r.m[0][0] = 1.0f;
    r.m[1][1] = 1.0f;
    r.m[2][2] = 1.0f;
    r.m[3][3] = 1.0f;
    return r;
}

/* Multiplies two 4x4 matrices: result = a * b */
Mat4 mat4Mul(Mat4 a, Mat4 b) {
    Mat4 r = { 0 };

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k][row] * b.m[col][k];
            }
            r.m[col][row] = sum;
        }
    }

    return r;
}

/*
 * Perspective projection matrix.
 * fovY   - vertical field of view in radians
 * aspect - width / height
 * near   - near clip plane
 * far    - far clip plane
 */
Mat4 mat4Perspective(float fovY, float aspect, float near, float far) {
    Mat4 r = { 0 };
    float tanHalf = tanf(fovY * 0.5f);

    r.m[0][0] = 1.0f / (aspect * tanHalf);
    r.m[1][1] = -1.0f / tanHalf;          /* flip Y for Vulkan NDC */
    r.m[2][2] = far / (near - far);
    r.m[2][3] = -1.0f;
    r.m[3][2] = -(far * near) / (far - near);

    return r;
}

/* Translation matrix: moves by (x, y, z) */
Mat4 mat4Translate(float x, float y, float z) {
    Mat4 r = mat4Identity();

    r.m[3][0] = x;
    r.m[3][1] = y;
    r.m[3][2] = z;

    return r;
}

/*
 * Rotation matrix from pitch, yaw, and roll in radians.
 * Pitch = rotation around X axis
 * Yaw   = rotation around Y axis
 * Roll  = rotation around Z axis
 */
Mat4 mat4Rotate(float pitch, float yaw, float roll) {
    float cp = cosf(pitch), sp = sinf(pitch);
    Mat4 rx = mat4Identity();
    rx.m[1][1] = cp;
    rx.m[1][2] = -sp;
    rx.m[2][1] = sp;
    rx.m[2][2] = cp;

    float cy = cosf(yaw), sy = sinf(yaw);
    Mat4 ry = mat4Identity();
    ry.m[0][0] = cy;
    ry.m[0][2] = -sy;
    ry.m[2][0] = sy;
    ry.m[2][2] = cy;

    float cr = cosf(roll), sr = sinf(roll);
    Mat4 rz = mat4Identity();
    rz.m[0][0] = cr;
    rz.m[0][1] = sr;
    rz.m[1][0] = -sr;
    rz.m[1][1] = cr;

    /* Combined rotation: Rz * Ry * Rx */
    return mat4Mul(mat4Mul(rz, ry), rx);
}

Mat4 mat4LookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = vec3Normalize(vec3Sub(target, eye));   // forward
    Vec3 s = vec3Normalize(vec3Cross(f, up));       // right
    Vec3 u = vec3Cross(s, f);                       // corrected up

    Mat4 r = mat4Identity();

    // column 0
    r.m[0][0] = s.x;
    r.m[0][1] = u.x;
    r.m[0][2] = -f.x;
    r.m[0][3] = 0.0f;

    // column 1
    r.m[1][0] = s.y;
    r.m[1][1] = u.y;
    r.m[1][2] = -f.y;
    r.m[1][3] = 0.0f;

    // column 2
    r.m[2][0] = s.z;
    r.m[2][1] = u.z;
    r.m[2][2] = -f.z;
    r.m[2][3] = 0.0f;

    // column 3
    r.m[3][0] = -vec3Dot(s, eye);
    r.m[3][1] = -vec3Dot(u, eye);
    r.m[3][2] = vec3Dot(f, eye);
    r.m[3][3] = 1.0f;

    return r;
}