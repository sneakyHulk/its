#include <pylon/PylonIncludes.h>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace Pylon;
using namespace std;

int main()
{
    // Initialize Pylon runtime system
    PylonInitialize();

    try {
        // Create an instant camera object for the first camera found
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // Open the camera
        camera.Open();

        // Get camera resolution for video encoding (width and height)
        int width = 2448;
        int height = 2048;

        // Set up OpenCV video writer (encode as MJPEG, for example)
        cv::VideoWriter writer("output.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, cv::Size(width, height));

        if (!writer.isOpened()) {
            cerr << "Error opening video writer!" << endl;
            return -1;
        }

        // Start grabbing images from the camera
        camera.StartGrabbing(GrabStrategy_OneByOne);

        // Loop to grab images and write them to video
        while (camera.IsGrabbing()) {
            // Retrieve the grabbed image
            CGrabResultPtr ptrGrabResult;
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);


            // Check if the image is valid
            if (ptrGrabResult->GrabSucceeded()) {
                // Convert the image to a format OpenCV understands (RGB)
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cv::Mat frame(height, width, CV_8UC3, (void*)pImageBuffer);

                // Convert from BGR to RGB if needed
                cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

                // Write the frame to the video file
                writer.write(frame);
            } else {
                cerr << "Error grabbing image!" << endl;
                break;
            }
        }

        // Stop grabbing and close the camera
        camera.StopGrabbing();
        camera.Close();

        cout << "Video saved as 'output.avi'" << endl;
    }
    catch (const GenericException& e) {
        cerr << "An exception occurred: " << e.GetDescription() << endl;
        PylonTerminate();
        return 1;
    }

    // Release the Pylon runtime system
    PylonTerminate();

    return 0;
}