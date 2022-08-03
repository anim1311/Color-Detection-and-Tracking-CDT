#include<iostream>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/core/core.hpp>

using namespace std;
using namespace cv;


//window properties

const int height = 480;
const int width = 640;

//Initial horizontal and vertical positions
int Horizontal_Last = -1;
int vertical_Last = -1;


//limits for the adjust window ( I have set it default to check for red color of the cgi book)
int Hue_Low = 0;
int Hue_high = 255;
int Sat_Low = 106;
int Sat_high = 255;
int Val_Low = 170;
int Val_high = 255;


bool colorChosen = 0;
bool run = 1;

int main(int argc, char** argv) {
    
    VideoCapture video(0);//capturing video from default camera

    video.set(CAP_PROP_FRAME_WIDTH, height);
    video.set(CAP_PROP_FRAME_HEIGHT, width);
    
    
    namedWindow("Adjust");
    
    createTrackbar("LowH", "Adjust", &Hue_Low, 179);
    createTrackbar("HighH", "Adjust", &Hue_high, 179);
    createTrackbar("LowS", "Adjust", &Sat_Low, 255);
    createTrackbar("HighS", "Adjust", &Sat_high, 255);
    createTrackbar("LowV", "Adjust", &Val_Low, 255);
    createTrackbar("HighV", "Adjust", &Val_high, 255);

    
    Mat temp;
    video.read(temp);

    // Time for a black matrix
    Mat track_motion = Mat::zeros(temp.size(), CV_8UC3);

    if (!video.isOpened()) {
        cout << "No video stream detected" << endl;
        system("pause");
        return -1;
    }
    
    while (run) {

        Mat actual_Image;
        bool temp_load = video.read(actual_Image);
        if (actual_Image.empty()) { //Breaking the loop if no video frame is detected
            cout << "No Video frame is detected" << endl;
            break;
        }

        Mat converted_to_HSV;
        cvtColor(actual_Image, converted_to_HSV, COLOR_BGR2HSV);//converting BGR image to HSV
        
        Mat adjusted_frame;//declaring a matrix to detected color
        inRange(converted_to_HSV, Scalar(Hue_Low, Sat_Low, Val_Low),
            Scalar(Hue_high, Sat_high, Val_high), adjusted_frame);//applying change of values of track-bars        
        
        erode(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));//morphological opening for removing small objects from foreground
        dilate(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));//morphological opening for removing small object from foreground
        dilate(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));//morphological closing for filling up small holes in foreground
        erode(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));//morphological closing for filling up small holes in foreground
       
        // The famous camshift algorithm
        Moments detecting_object = moments(adjusted_frame);
        double vertical_moment = detecting_object.m01;
        double horizontal_moment = detecting_object.m10;
        double tracking_area = detecting_object.m00;
        
        if (tracking_area > 10000) { 

            //calculating vertical and horitzontal postions
            int posX = horizontal_moment / tracking_area;
            int posY = vertical_moment / tracking_area;

            
            if (Horizontal_Last >= 0 && vertical_Last >= 0 && posX >= 0 && posY >= 0 && colorChosen) { //when the detected object moves
                line(track_motion, Point(posX, posY), Point(Horizontal_Last, vertical_Last), Scalar(255, 0, 0), 2);
                /*
                //Prepare the image for findContours
                Mat temp2;
                cvtColor(adjusted_frame, temp, COLOR_BGR2GRAY);
                adjusted_frame = temp2.clone();
                threshold(adjusted_frame, temp, 200, 255, THRESH_BINARY);
                adjusted_frame = temp2.clone();
                
                //Find the contours
                vector<vector<Point>> contours;
                Mat contourOutput = adjusted_frame.clone();
                findContours(contourOutput, contours, RETR_LIST, CHAIN_APPROX_NONE);

                //Draw the contours
                Scalar colors[3];
                Mat contourImage(adjusted_frame.size(), CV_8UC3, cv::Scalar(0, 0, 0));
                colors[0] = Scalar(255, 0, 0);
                colors[1] = Scalar(0, 255, 0);
                colors[2] = Scalar(0, 0, 255);
                for (size_t idx = 0; idx < contours.size(); idx++) {
                    cv::drawContours(adjusted_frame, contours, idx, colors[idx % 3]);
                }
                imshow("Contours", contourImage);
                
                moveWindow("Contours", 200, 0);
                */
            }
            // Getting new horizontal and vertical values
            Horizontal_Last = posX;
            vertical_Last = posY; 
        }
        
        imshow("Detected_Object", adjusted_frame);//showing detected object
       
        actual_Image = actual_Image + track_motion;//drawing continuous line in original video frames
       
        imshow("Actual", actual_Image);//showing original video
       

        cout << "position of the object is:" << Horizontal_Last << "," << vertical_Last << endl;//showing tracked co-ordinated values
        
        int key = waitKey(25)%256;
        
        switch (key)
        {
        
        case ((int)'a'):
            colorChosen |= 1;
            break;
        case 27:// this is escape in unicode
            run &= 0;
        default:
            break;
        }
        

    }
    return 0;
}