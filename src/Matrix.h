

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE



class Matrix {
public:
    // constructor
    Matrix() {
        // printf("Executing constructor Matrix() ...\n");
        // create a Matrix object without content
        p = NULL;
        rows = 0;
        cols = 0;
    }
Matrix(const int row_count, const int column_count);
Matrix Diag(const int n);
Matrix Diag(const Matrix& v);
Matrix(const Matrix& a);

Matrix Ones(const int rows, const int cols);
Matrix Zeros(const int rows, const int cols);
void Extend(int row, int col);
double& operator()(const int r, const int c);
double get(const int r, const int c) const;
Matrix& Add(const double v);
// subtract a double value (elements wise)
Matrix& Subtract(const double v) { return Add(-v); }
Matrix& Multiply(const double v);
friend Matrix operator+(const Matrix& a, const Matrix& b);
friend Matrix operator+(const Matrix& a, const double b);
friend Matrix operator+(const double b, const Matrix& a);
friend Matrix operator-(const Matrix& a, const Matrix& b);
friend Matrix operator-(const Matrix& a, const double b);
friend Matrix operator-(const double b, const Matrix& a);
friend Matrix operator-(const Matrix& a);
friend Matrix operator*(const Matrix& a, const Matrix& b);
friend Matrix operator*(const Matrix& a, const double b);
friend Matrix operator*(const double b, const Matrix& a);
Matrix& operator=(const Matrix& a);

int Size(const int i) const;
// returns the number of rows
int GetRows() const { return rows; }
// returns the number of columns
int GetCols() const { return cols; }
Matrix Minor(const int row, const int col) const;

// destructor
~Matrix() {
    // clean up allocated memory
    if (p != NULL){   // added DF
        for (int r = 0; r < rows; r++) {
            delete p[r];
        }
        delete p;
        p = NULL;
    }
}


    int rows;
    int cols;
    double** p;  // pointer to a matrix with doubles
};

Matrix Inv(const Matrix& a);
double Det(const Matrix& a);

PLUGIN_END_NAMESPACE
#endif 