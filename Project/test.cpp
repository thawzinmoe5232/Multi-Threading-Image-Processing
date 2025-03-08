#include <opencv2/opencv.hpp>
#include <iostream>

// Function to load an image from a file path
cv::Mat loadImage(const std::string& filepath) {
    cv::Mat img = cv::imread(filepath);
    if (img.empty()) {
        std::cerr << "Error: Could not load image. Check the file path!" << std::endl;
        exit(-1);
    }
    return img;
}

// Function to apply the median filter to an image
cv::Mat applyMedianFilter(const cv::Mat& img, int kernelSize) {
    cv::Mat filtered_img;
    cv::medianBlur(img, filtered_img, kernelSize);  // Apply median filter
    return filtered_img;
}

// Function to apply the Laplacian filter to an image
cv::Mat applyLaplacianFilter(const cv::Mat& img, int kernelSize) {
    cv::Mat filtered_img;
    cv::Laplacian(img, filtered_img, CV_8U, kernelSize);  // Apply Laplacian filter
    return filtered_img;
}

// Function to apply the Gaussian filter to an image
cv::Mat applyGaussianFilter(const cv::Mat& img, const cv::Size& kernelSize, double sigmaX) {
    cv::Mat filtered_img;
    cv::GaussianBlur(img, filtered_img, kernelSize, sigmaX);  // Apply Gaussian filter
    return filtered_img;
}

// Function to display an image in a window and adjust the window size to match the image size
void displayImage(const std::string& windowName, const cv::Mat& img) {
    cv::namedWindow(windowName, cv::WINDOW_NORMAL); // Create a window that can be resized
    cv::resizeWindow(windowName, img.cols, img.rows); // Set the window size to the image size
    cv::imshow(windowName, img); // Display the image
    cv::waitKey(0);  // Wait for a key press
}

// Function to check if images are equal, considering potential size differences due to filtering
bool areImagesEqual(const cv::Mat& img1, const cv::Mat& img2) {
    // Check if images are the same size
    if (img1.size() != img2.size()) {
        return false; // If sizes are different, they are not equal
    }

    // Compare pixel-by-pixel, ignoring size differences
    return cv::countNonZero(img1 != img2) == 0;
}

int main() {
    // Load the image
    cv::Mat img = loadImage("image.jpg");

    // Display the original image
    displayImage("Original Image", img);

    // Ask user which filter they want to apply
    int choice;
    std::cout << "Select the filter to apply:" << std::endl;
    std::cout << "1. Median Filter" << std::endl;
    std::cout << "2. Laplacian Filter" << std::endl;
    std::cout << "3. Gaussian Filter" << std::endl;
    std::cout << "Enter your choice (1/2/3): ";
    std::cin >> choice;

    int kernelSize, recursionLevel;
    cv::Mat filtered_img;

    // Ask for kernel size and recursion level
    std::cout << "Enter kernel size (odd number): ";
    std::cin >> kernelSize;
    std::cout << "Enter recursion level (0 for no recursion): ";
    std::cin >> recursionLevel;

    // Ask for sigmaX for Gaussian filter
    double sigmaX = 0;
    if (choice == 3) {
        std::cout << "Enter sigmaX for Gaussian filter: ";
        std::cin >> sigmaX;
    }

    // Apply the selected filter based on user input
    switch (choice) {
        case 1:
            // Apply Median Filter
            filtered_img = applyMedianFilter(img, kernelSize);
            break;

        case 2:
            // Apply Laplacian Filter
            filtered_img = applyLaplacianFilter(img, kernelSize);
            break;

        case 3:
            // Apply Gaussian Filter
            filtered_img = applyGaussianFilter(img, cv::Size(kernelSize, kernelSize), sigmaX);
            break;

        default:
            std::cerr << "Invalid choice! Exiting program..." << std::endl;
            return -1;
    }

    // Display the filtered image
    displayImage("Filtered Image", filtered_img);



    // The areImagesEqual function is broken

    
    // Check if the images are the same
    /*if (areImagesEqual(img, filtered_img)) {
        std::cout << "The input image is the same as the final image." << std::endl;
    } else {
        std::cout << "The input image is different from the final image." << std::endl;
    }
    */
    return 0;
}
