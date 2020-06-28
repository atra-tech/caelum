#include <math.h>

#include "matrix.h"
#include "../util/util.h"
#include "../util/log.h"

Matrix *
matrixcreate(int width, int height)
{
    Matrix *mat = (Matrix *) malloc(sizeof(Matrix));
    if (checkmalloc(mat) < 0) {
        logprint(LOG_ERROR, "matrix: Unable to create matrix: Memory error");
        return NULL;
    }

    mat->data = (float *) malloc(width * height * sizeof(float));
    if (checkmalloc(mat->data) < 0) {
        logprint(LOG_ERROR, "matrix: Unable to create matrix: Memory error");
        return NULL;
    }
    mat->width = width;
    mat->height = height;

    return mat;
}

void
matrixdelete(Matrix *mat)
{
    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to delete matrix: " \
                            "NULL matrix pointer received");
        return;
    }
    free(mat->data);
    free(mat);
    mat = NULL;
}

void
matrixnormalise(float *x, float *y, float *z) {
    float d = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
    *x /= d;
    *y /= d;
    *z /= d;
}

void
matrixidentity(Matrix *mat) {
    int i, j;

    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to set identity matrix: " \
                            "NULL matrix pointer received");
        return;
    } else if (mat->width != mat->height) {
        logprint(LOG_ERROR, "matrix: Unable to set identity matrix: " \
                            "Matrix width and height are not equal");
        return;
    }

    for (i = 0; i < mat->width; i++) {
        for (j = 0; j < mat->height; j++) {
            if (i == j) {
                mat->data[j + i * mat->width] = 1;
            } else {
                mat->data[j + i * mat->width] = 0;
            }
        }
    }
}

void
matrixtranslate(Matrix *mat, float *ds)
{
    int i, j;

    if (!mat || !ds) {
        logprint(LOG_ERROR, "matrix: Unable to set translation matrix: " \
                            "NULL pointer received");
        return;
    } else if (mat->width != mat->height) {
        logprint(LOG_ERROR, "matrix: Unable to set translation matrix: " \
                            "Matrix width and height are not equal");
        return;
    }

    for(i = 0; i < mat->width; i++) {
        for(j = 0; j < mat->height; j++) {
            if (i == j) {
                mat->data[j + i * mat->width] = 1;
            } else if (i == mat->width - 1) {
                mat->data[j + i * mat->width] = ds[j];
            } else {
                mat->data[j + i * mat->width] = 0;
            }
        }
    }
}

void
matrixrotate(Matrix *mat, float x, float y, float z, float theta)
{
    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to set rotation matrix: " \
                            "NULL matrix pointer received");
        return;
    } else if (mat->width != 4 || mat->height != 4) {
        logprint(LOG_ERROR, "matrix: Unable to set rotation matrix: " \
                            "Matrix dimensions are not 4x4");
        return;
    }
    matrixnormalise(&x, &y, &z);
    float s = sinf(theta);
    float c = cosf(theta);
    float m = 1 - c;
    mat->data[0] = m * x * x + c;
    mat->data[1] = m * x * y - z * s;
    mat->data[2] = m * z * x + y * s;
    mat->data[3] = 0;
    mat->data[4] = m * x * y + z * s;
    mat->data[5] = m * y * y + c;
    mat->data[6] = m * y * z - x * s;
    mat->data[7] = 0;
    mat->data[8] = m * z * x - y * s;
    mat->data[9] = m * y * z + x * s;
    mat->data[10] = m * z * z + c;
    mat->data[11] = 0;
    mat->data[12] = 0;
    mat->data[13] = 0;
    mat->data[14] = 0;
    mat->data[15] = 1;
}

void
matrixvecmult(Matrix *vec, Matrix *mat_a, Matrix *vec_b)
{
    int i, j, p, q;
    float total;

    if (!vec || !mat_a || !vec_b) {
        logprint(LOG_ERROR, "matrix: Unable to multiply vector: " \
                            "NULL pointer received");
        return;
    } else if (vec->width != 1 || vec->height < 2 ||
        vec_b->width != 1 || vec_b->height < 2) {
        logprint(LOG_ERROR, "matrix: Unable to multiply vector: " \
                            "Vector has incorrect dimensions");
        return;
    } else if (mat_a->height != vec_b->height) {
        logprint(LOG_ERROR, "matrix: Unable to multiply vector: " \
                            "Vector and matrix heights do not match");
        return;
    }
    
    for (i = 0; i < mat_a->width; i++) {
        total = 0;
        for (j = 0; j < mat_a->height; j++) {
            p = j * 4 + i;
            q = j;
            total += mat_a->data[p] * vec_b->data[q];
        }
        vec->data[i] = total;
    }
}

void
vectorcross(Matrix *vec, Matrix *vec_a, Matrix *vec_b)
{
    if (!vec || !vec_a || !vec_b) {
        logprint(LOG_ERROR, "matrix: Unable to get vector cross product: " \
                            "NULL pointer received");
        return;
    } else if (vec->height != 3 || vec_a->height != 3 || vec_b->height != 3) {
        logprint(LOG_ERROR, "matrix: Unable to get vector cross product: " \
                            "Vectors do not both have height 3");
        return;
    } else if (vec->width != 1 || vec_a->width != 1 || vec_b->width != 1) {
        logprint(LOG_ERROR, "matrix: Unable to get vector cross product: " \
                            "Vectors do not both have width 1");
        return;
    }
    vec->data[0] = vec_a->data[1] * vec_b->data[2] -
                   vec_a->data[2] * vec_b->data[1];
    vec->data[1] = vec_a->data[2] * vec_b->data[0] -
                   vec_a->data[0] * vec_b->data[2];
    vec->data[2] = vec_a->data[0] * vec_b->data[1] -
                   vec_a->data[1] * vec_b->data[0];
}

float
vectordot(Matrix *vec_a, Matrix *vec_b)
{
    if (!vec_a || !vec_b) {
        logprint(LOG_ERROR, "matrix: Unable to get vector dot product: " \
                            "NULL pointer received");
        return 0;
    } else if (vec_a->height != 3 || vec_b->height != 3) {
        logprint(LOG_ERROR, "matrix: Unable to get vector dot product: " \
                            "Vectors do not both have height 3");
        return 0;
    } else if (vec_a->width != 1 || vec_b->width != 1) {
        logprint(LOG_ERROR, "matrix: Unable to get vector dot product: " \
                            "Vectors do not both have width 1");
        return 0;
    }
    return vec_a->data[0] * vec_b->data[0] +
           vec_a->data[1] * vec_b->data[1] +
           vec_a->data[2] * vec_b->data[2];
}

void
matrixmult(Matrix *mat, Matrix *mat_a, Matrix *mat_b)
{
    int c, r, i, p, q, index;
    float total;

    if (!mat || !mat_a || !mat_b) {
        logprint(LOG_ERROR, "matrix: Unable to multiply matrices: " \
                            "NULL matrix pointer received");
        return;
    } else if (mat_a->height != mat_b->height ||
               mat_a->width != mat_b->width) {
        logprint(LOG_ERROR, "matrix: Unable to multiply matrices: " \
                            "Matrices do not have the same dimensions");
        return;
    } else if (mat_a->height != mat_a->width) {
        logprint(LOG_ERROR, "matrix: Unable to multiply matrices: " \
                            "Matrices are not of dimensions NxN");
        return;
    }
    
    for (c = 0; c < mat_a->width; c++) {
        for (r = 0; r < mat_a->height; r++) {
            index = c * mat_a->width + r;
            total = 0;
            for (i = 0; i < mat_a->height; i++) {
                p = i * mat_a->height + r;
                q = c * mat_a->height + i;
                total += mat_a->data[p] * mat_b->data[q];
            }
            mat->data[index] = total;
        }
    }
}

void
matrixapply(float *data, Matrix *mat, int count, int offset, int stride)
{
    int i;
    float *d;
    float vec_data[4] = {0, 0, 0, 1};
    Matrix vec;

    vec.data = vec_data;
    vec.width = 1; vec.height = 4;
    for (i = 0; i < 4; i++) 
        vec.data[i] = vec_data[i];

    for (i = 0; i < count; i++) {
        d = data + offset + stride * i;
        vec.data[0] = *(d++); 
        vec.data[1] = *(d++);
        vec.data[2] = *(d++);
        matrixvecmult(&vec, mat, &vec);
        d = data + offset + stride * i;
        *(d++) = vec.data[0];
        *(d++) = vec.data[1];
        *(d++) = vec.data[2];
    }
}

void
frustumplanes(float planes[6][4], int radius, Matrix *mat)
{
    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to set frustum planes: " \
                            "NULL matrix pointer received");
        return;
    } else if (mat->width != 4 || mat->height != 4) {
        logprint(LOG_ERROR, "matrix: Unable to set frustum planes: " \
                            "Given matrix is not 4x4");
    }
    float znear = 0.125;
    float zfar = radius * 32 + 64;
    float *m = mat->data;
    planes[0][0] = m[3] + m[0];
    planes[0][1] = m[7] + m[4];
    planes[0][2] = m[11] + m[8];
    planes[0][3] = m[15] + m[12];
    planes[1][0] = m[3] - m[0];
    planes[1][1] = m[7] - m[4];
    planes[1][2] = m[11] - m[8];
    planes[1][3] = m[15] - m[12];
    planes[2][0] = m[3] + m[1]; 
    planes[2][1] = m[7] + m[5];
    planes[2][2] = m[11] + m[9];
    planes[2][3] = m[15] + m[13];
    planes[3][0] = m[3] - m[1];
    planes[3][1] = m[7] - m[5];
    planes[3][2] = m[11] - m[9];
    planes[3][3] = m[15] - m[13];
    planes[4][0] = znear * m[3] + m[2];
    planes[4][1] = znear * m[7] + m[6];
    planes[4][2] = znear * m[11] + m[10];
    planes[4][3] = znear * m[15] + m[14];
    planes[5][0] = zfar * m[3] - m[2];
    planes[5][1] = zfar * m[7] - m[6];
    planes[5][2] = zfar * m[11] - m[10];
    planes[5][3] = zfar * m[15] - m[14];
}

void
matrixfrustum(Matrix *mat, float left, float right, float bottom,
              float top, float znear, float zfar)
{
    float temp, temp2, temp3, temp4;
    
    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to set frustum matrix: " \
                            "NULL matrix pointer received");
        return;
    } else if (mat->width != 4 || mat->height != 4) {
        logprint(LOG_ERROR, "matrix: Unable to set frustum matrix: " \
                            "Given matrix is not 4x4");
    }
    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    mat->data[0] = temp / temp2;
    mat->data[1] = 0.0f;
    mat->data[2] = 0.0f;
    mat->data[3] = 0.0f;
    mat->data[4] = 0.0f;
    mat->data[5] = temp / temp3;
    mat->data[6] = 0.0f;
    mat->data[7] = 0.0f;
    mat->data[8] = (right + left) / temp2;
    mat->data[9] = (top + bottom) / temp3;
    mat->data[10] = (-zfar - znear) / temp4;
    mat->data[11] = -1.0f;
    mat->data[12] = 0.0f;
    mat->data[13] = 0.0f;
    mat->data[14] = (-temp * zfar) / temp4;
    mat->data[15] = 0.0f;
}

void
matrixlookat(Matrix *mat, Matrix *eye, Matrix *centre, Matrix *up)
{
    int i;
    Matrix *vec_x, *vec_y, *vec_z;

    if (!mat || !eye || !centre || !up) {
        logprint(LOG_ERROR, "matrix: Unable to set lookat matrix: " \
                            "NULL pointer received");
        return;
    } else if (eye->height != 3 || 
               centre->height != 3 ||
               up->height != 3) {
        logprint(LOG_ERROR, "matrix: Unable to set lookat matrix: " \
                            "Vectors do not both have height 3");
        return;
    } else if (eye->width != 1 || 
               centre->width != 1 ||
               up->width != 1) {
        logprint(LOG_ERROR, "matrix: Unable to set lookat matrix: " \
                            "Vectors do not both have width 1");
        return;
    } else if (mat->width != 4 || mat->height != 4) {
        logprint(LOG_ERROR, "matrix: Unable to set lookat matrix: " \
                            "Matrix is not 4x4");
        return;
    }
    vec_x = matrixcreate(1, 3);
    vec_y = matrixcreate(1, 3);
    vec_z = matrixcreate(1, 3);

    for (i = 0; i < 3; i++)
        vec_z->data[i] = eye->data[i] - centre->data[i];

    matrixnormalise(&vec_z->data[0], &vec_z->data[1], &vec_z->data[2]);

    for (i = 0; i < 3; i++)
        vec_y->data[i] = up->data[i];

    vectorcross(vec_x, vec_y, vec_z);
    vectorcross(vec_y, vec_z, vec_x);
    matrixnormalise(&vec_x->data[0], &vec_x->data[1], &vec_x->data[2]);
    matrixnormalise(&vec_y->data[0], &vec_y->data[1], &vec_y->data[2]);

    mat->data[0] = vec_x->data[0];
    mat->data[1] = vec_x->data[1];
    mat->data[2] = vec_x->data[2];
    mat->data[3] = -vectordot(vec_x, eye);
    mat->data[4] = vec_y->data[0];
    mat->data[5] = vec_y->data[1];
    mat->data[6] = vec_y->data[2];
    mat->data[7] = -vectordot(vec_y, eye);
    mat->data[8] = vec_z->data[0];
    mat->data[9] = vec_z->data[1];
    mat->data[10] = vec_z->data[2];
    mat->data[11] = -vectordot(vec_z, eye);
    mat->data[12] = 0.0f; 
    mat->data[13] = 0.0f;
    mat->data[14] = 0.0f;
    mat->data[15] = 1.0f;

    matrixdelete(vec_x);
    matrixdelete(vec_y);
    matrixdelete(vec_z);
}

void
matrixperspective(Matrix *mat, float fov, float aspect,
                  float znear, float zfar)
{
    float ymax, xmax;

    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to set perspective matrix: " \
                            "NULL matrix pointer received");
        return;
    }
    ymax = znear * tanf(fov * PI / 360.0f);
    xmax = ymax * aspect;
    matrixfrustum(mat, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void
matrixortho(Matrix *mat, float left, float right, float bottom,
            float top, float near, float far)
{    
    if (!mat) {
        logprint(LOG_ERROR, "matrix: Unable to set orthographic matrix: " \
                            "NULL matrix pointer received");
        return;
    } else if (mat->width != 4 || mat->height != 4) {
        logprint(LOG_ERROR, "matrix: Unable to set orthographic matrix: " \
                            "Given matrix is not 4x4");
    }
    mat->data[0] = 2 / (right - left);
    mat->data[1] = 0;
    mat->data[2] = 0;
    mat->data[3] = 0;
    mat->data[4] = 0;
    mat->data[5] = 2 / (top - bottom);
    mat->data[6] = 0;
    mat->data[7] = 0;
    mat->data[8] = 0;
    mat->data[9] = 0;
    mat->data[10] = -2 / (far - near);
    mat->data[11] = 0;
    mat->data[12] = -(right + left) / (right - left);
    mat->data[13] = -(top + bottom) / (top - bottom);
    mat->data[14] = -(far + near) / (far - near);
    mat->data[15] = 1;
}

void
matrix2d(Matrix *mat, int width, int height)
{
    matrixortho(mat, 0, width, 0, height, -1, 1);
}

void
matrix3d(Matrix *mat, int width, int height, float x, float y, float z, 
         float rx, float ry, float fov, int ortho, int radius)
{
    Matrix *a = matrixcreate(4, 4);
    Matrix *b = matrixcreate(4, 4);
    float aspect = (float)width / height;
    float znear = 0.125;
    float zfar = radius * 32 + 64;
    float data[3] = {x, y, z};

    if (!mat) {
        logprint(LOG_ERROR, "matrix: Could not make 3D matrix: " \
                            "NULL matrix pointer received");
        return;
    }
    matrixidentity(a);
    matrixtranslate(b, data);
    matrixmult(a, b, a);
    matrixrotate(b, cosf(rx), 0, sinf(rx), ry);
    matrixmult(a, b, a);
    matrixrotate(b, 0, 1, 0, -rx);
    matrixmult(a, b, a);
    if (ortho) {
        int size = ortho;
        matrixortho(
            b, -size * aspect, size * aspect, -size, size, -zfar, zfar);
    } else {
        matrixperspective(b, fov, aspect, znear, zfar);
    }
    matrixmult(a, b, a);
    matrixidentity(mat);
    matrixmult(mat, a, mat);

    matrixdelete(a);
    matrixdelete(b);
}
