/*
 *  ATRA ENGINE (2019)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Description:
 *  Matrix structure and some associated operations.
 *  Vectors are also provided here.
 *
 *  */

#ifndef ATRA_MATRIX_H
#define ATRA_MATRIX_H

typedef struct {
    float *data;
    int width;
    int height;
} Matrix;

Matrix *matrixcreate(int width, int height);
void matrixdelete(Matrix *mat);
void matrixnormalise(float *x, float *y, float *z);
void matrixidentity(Matrix *mat);
void matrixtranslate(Matrix *mat, float *ds);
void matrixrotate(Matrix *mat, float x, float y, float z, float theta);
void matrixvecmult(Matrix *vec, Matrix *mat_a, Matrix *vec_b);
void vectorcross(Matrix *vec, Matrix *vec_a, Matrix *vec_b);
float vectordot(Matrix *vec_a, Matrix *vec_b);
void matrixmult(Matrix *mat, Matrix *mat_a, Matrix *mat_b);
void matrixapply(float *data, Matrix *mat, int count, int offset, int stride);
void frustumplanes(float planes[6][4], int radius, Matrix *mat);
void matrixfrustum(Matrix *mat, float left, float right, float bottom,
                   float top, float znear, float zfar);
void matrixlookat(Matrix *mat, Matrix *eye, Matrix *centre, Matrix *up);
void matrixperspective(Matrix *mat, float fov, float aspect,
                       float znear, float zfar);
void matrixortho(Matrix *mat, float left, float right, float bottom,
                 float top, float near, float far);
void matrix2d(Matrix *mat, int width, int height);
void matrix3d(Matrix *mat, int width, int height, float x, float y, float z,
              float rx, float ry, float fov, int ortho, int radius);

#endif
