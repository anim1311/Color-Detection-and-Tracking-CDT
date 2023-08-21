#include "orientation.h"

#define PRINTOBJLOC 0
#define PRINTCOLORDATA 1

// window properties

const int height = 480;
const int width = 640;

// Initial horizontal and vertical positions
int Horizontal_Last = -1;
int vertical_Last = -1;

// limits for the adjust window ( I have set it default to check for red color of the cgi book)
int Hue_Low = 0;
int Hue_high = 255;
int Sat_Low = 0;
int Sat_high = 255;
int Val_Low = 0;
int Val_high = 255;

// control Variables
bool colorChosen = 0;
bool run = 1;
bool trackOrientation = 0;
char blend = 12;

Vec3d rgb_to_hsv(Vec3d color)
{

    // R, G, B values are divided by 255
    // to change the range from 0..255 to 0..1
    color[2] = color[2] / 255.0;
    color[1] = color[1] / 255.0;
    color[0] = color[0] / 255.0;

    // h, s, v = hue, saturation, value
    double cmax = max(color[2], max(color[1], color[0])); // maximum of r, g, b
    double cmin = min(color[0], min(color[1], color[0])); // minimum of r, g, b
    double diff = cmax - cmin;                            // diff of cmax and cmin.
    double h = -1, s = -1;

    // if cmax and cmax are equal then h = 0
    if (cmax == cmin)
        h = 0;

    // if cmax equal r then compute h
    else if (cmax == color[2])
        h = fmod(60 * ((color[1] - color[0]) / diff) + 360, 360);

    // if cmax equal g then compute h
    else if (cmax == color[1])
        h = fmod(60 * ((color[0] - color[2]) / diff) + 120, 360);

    // if cmax equal b then compute h
    else if (cmax == color[0])
        h = fmod(60 * ((color[2] - color[1]) / diff) + 240, 360);

    // if cmax equal zero
    if (cmax == 0)
        s = 0;
    else
        s = (diff / cmax) * 100;

    // compute v
    double v = cmax * 100;

    return Vec3d({h, s, v});
}

void onMouse(int evt, int x, int y, int flags, void *param)
{

    if (evt == cv::EVENT_FLAG_LBUTTON)
    {

        Mat *mtPtr = (Mat *)param;
        Vec3b color = mtPtr->at<Vec3i>(x, y);
        cout << color << endl;
        color = rgb_to_hsv(color);
        cout << color << endl;
        Hue_Low = color[0];
        Sat_Low = color[1];
        Val_Low = color[2];
    }
}
void createLoadMenu()
{
    namedWindow("Adjust");

    createTrackbar("LowH", "Adjust", &Hue_Low, 179);
    createTrackbar("HighH", "Adjust", &Hue_high, 179);
    createTrackbar("LowS", "Adjust", &Sat_Low, 255);
    createTrackbar("HighS", "Adjust", &Sat_high, 255);
    createTrackbar("LowV", "Adjust", &Val_Low, 255);
    createTrackbar("HighV", "Adjust", &Val_high, 255);
}

int main(int argc, char **argv)
{

    VideoCapture video(0); // capturing video from default camera

    video.set(CAP_PROP_FRAME_WIDTH, height);
    video.set(CAP_PROP_FRAME_HEIGHT, width);

    createLoadMenu();

    Mat temp;
    video.read(temp);

    // Time for a black matrix
    Mat track_motion = Mat::zeros(temp.size(), CV_8UC3);

    if (!video.isOpened())
    {
        cout << "No video stream detected" << endl;
        system("pause");
        return -1;
    }

    while (run)
    {

        Mat actual_Image;
        bool temp_load = video.read(actual_Image);
        if (actual_Image.empty())
        { // Breaking the loop if no video frame is detected
            cout << "No Video frame is detected" << endl;
            break;
        }

        Mat converted_to_HSV;
        cvtColor(actual_Image, converted_to_HSV, COLOR_BGR2HSV); // converting BGR image to HSV

        Mat adjusted_frame; // declaring a matrix to detected color
        inRange(converted_to_HSV, Scalar(Hue_Low, Sat_Low, Val_Low),
                Scalar(Hue_high, Sat_high, Val_high), adjusted_frame); // applying change of values of track-bars

        erode(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend)));  // morphological opening for removing small objects from foreground
        dilate(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend))); // morphological opening for removing small object from foreground
        dilate(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend))); // morphological closing for filling up small holes in foreground
        erode(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend)));  // morphological closing for filling up small holes in foreground

        if (trackOrientation)
        {
            // copying tracking image
            Mat t;
            adjusted_frame.copyTo(t);

            // converting image to binary
            Mat bin;
            threshold(t, bin, 50, 255, THRESH_BINARY | THRESH_OTSU);

            // Find all the contorus in the threshold image
            vector<vector<Point>> contours;
            findContours(bin, contours, RETR_LIST, CHAIN_APPROX_NONE);

            double angle = 0;

            for (int i = 0; i < contours.size(); i++)
            {
                double area = contourArea(contours[i]);

                if (area < 1e2 || 1e5 < area)
                    continue;

                drawContours(t, contours, i, Scalar(150, 150, 150), 2);

                angle = 180.0 * getOrientation(contours[i], t) / CV_PI;
            }

            putText(t, "Angle: " + std::to_string(angle), Point(10, t.rows / 2), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 1, 8);
            imshow("orientaionDisplay", t);
        }

        // The famous camshift algorithm
        Moments detecting_object = moments(adjusted_frame);
        double vertical_moment = detecting_object.m01;
        double horizontal_moment = detecting_object.m10;
        double tracking_area = detecting_object.m00;

        if (tracking_area > 10000)
        {

            // calculating vertical and horitzontal postions
            int posX = horizontal_moment / tracking_area;
            int posY = vertical_moment / tracking_area;

            if (Horizontal_Last >= 0 && vertical_Last >= 0 && posX >= 0 && posY >= 0 && colorChosen)
            { // when the detected object moves
                line(track_motion, Point(posX, posY), Point(Horizontal_Last, vertical_Last), Scalar(255, 255, 255), 2);
                circle(actual_Image, Point(posX, posY), 50, Scalar(255, 123, 123));
            }
            // Getting new horizontal and vertical values
            Horizontal_Last = posX;
            vertical_Last = posY;
        }

        imshow("Detected_Object", adjusted_frame); // showing detected object

        actual_Image = actual_Image + track_motion; // drawing continuous line in original video frames

        imshow("Actual", actual_Image); // showing original video

        setMouseCallback("Actual", onMouse, (void *)&converted_to_HSV); // for selection of color

#ifndef PRINTOBJLOC
        cout << "position of the object is:" << Horizontal_Last << "," << vertical_Last << endl; // showing tracked co-ordinated values
#endif
        int key = waitKey(25) % 256;

        switch (key)
        {

        case ((int)'a'):
            colorChosen = !colorChosen;
            break;
        case 27: // this is escape in unicode
            run &= 0;
        case ((int)'t'):
            trackOrientation = !trackOrientation;
        default:
            break;
        }
    }
    return 0;
}
