#include"orientation.h"

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
int Val_Low = 0;
int Val_high = 255;


// control Variables
bool colorChosen = 0;
bool run = 1;
bool trackOrientation = 0;
int blend = 6;






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
        
        erode(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend)));//morphological opening for removing small objects from foreground
        dilate(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend)));//morphological opening for removing small object from foreground
        dilate(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend)));//morphological closing for filling up small holes in foreground
        erode(adjusted_frame, adjusted_frame, getStructuringElement(MORPH_ELLIPSE, Size(blend, blend)));//morphological closing for filling up small holes in foreground
        
        if (trackOrientation) {
            // copying tracking image 
            Mat t;
            adjusted_frame.copyTo(t);

            // converting image to binary
            Mat bin;
            threshold(t, bin, 50, 255, THRESH_BINARY | THRESH_OTSU);

            //Find all the contorus in the threshold image
            vector<vector<Point>> contours;
            findContours(bin, contours, RETR_LIST, CHAIN_APPROX_NONE);
            
            double angle = 0;

            for (int i = 0; i < contours.size(); i++)
            {
                double area = contourArea(contours[i]);

                if (area < 1e2 || 1e5 < area) continue;

                drawContours(t, contours, i, Scalar(150, 150, 150), 2);

               angle= getOrientation(contours[i], t);
            }
            
            putText(t,"Angle: ", Point(0, 0), FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 1, 8, true);
            imshow("T", t);
        }
        

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
            colorChosen =!colorChosen;
            break;
        case 27:// this is escape in unicode
            run &= 0;
        case ((int)'t'):
            trackOrientation = !trackOrientation;
        default:
            break;
        }
        

    }
    return 0;
}





