#pragma once


#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>

using namespace std;
using namespace cv;

// Function declarations
void drawAxis(Mat&, Point, Point, Scalar, const float);
double getOrientation(const vector<Point>&, Mat&);