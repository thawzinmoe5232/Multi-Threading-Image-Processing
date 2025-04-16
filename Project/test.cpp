#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

// Function to load an image from a file path
cv::Mat loadImage(const std::string& filepath) {
    cv::Mat img = cv::imread(filepath);
    if (img.empty()) {
        std::cerr << "Error: Could not load image. Check the file path!" << std::endl;
        exit(-1);
    }
    return img;
}

// Function to load a kernel from a file path
std::tuple<int, cv::Mat> loadKernel(const std::string& filepath) {
    std::ifstream File(filepath);
    if (!File.is_open()) {
        std::cerr << "Error: Could not load kernel. Check the file path!" << std::endl;
        exit(-1);
    }

    int kernelSize;
    File >> kernelSize;

    cv::Mat kernel(kernelSize, kernelSize, CV_32F);
    for (int i = 0; i < kernelSize; i++) {
        for (int j = 0; j < kernelSize; j++) {
            float value;
            if (!(File >> value)) {
                std::cerr << "Error: kernel file is not formatted properly!" << std::endl;
                exit(-1);
            };
            kernel.at<float>(i, j) = value;
        }
    }
    File.close();
    return {kernelSize, kernel};
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

// Function to apply the custom filter to an image
cv::Mat applyCustomFilter(const cv::Mat& img, const cv::Mat& kernel) {
    cv::Mat filtered_img;
    cv::filter2D(img, filtered_img, -1, kernel); // Apply custom filter
    return filtered_img;
}

// Function to display an image in a window and adjust the window size to match the image size
void displayImage(const std::string& windowName, const cv::Mat& img) {
    cv::namedWindow(windowName, cv::WINDOW_NORMAL); // Create a window that can be resized
    cv::resizeWindow(windowName, img.cols, img.rows); // Set the window size to the image size
    cv::imshow(windowName, img); // Display the image
    cv::waitKey(0);  // Wait for a key press
}

// Function to split the image into horizontal sections (Mode 1)
void splitImageHorizontally(const cv::Mat& img, std::vector<cv::Rect>& regions, int numThreads) {
    int rows = img.rows;
    int cols = img.cols;

    // Calculate the height of each region
    int stepY = rows / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int y = i * stepY;
        int height = (i == numThreads - 1) ? rows - y : stepY;
        regions.push_back(cv::Rect(0, y, cols, height));
    }
}

// Recursive function to split the image into 4^n sections (Mode 2)
void splitImage4N(const cv::Mat& img, std::vector<cv::Rect>& regions, int level, const cv::Rect& region) {
    if (level == 0) {
        regions.push_back(region);
        return;
    }

    // Calculate new region size
    int halfWidth = region.width / 2;
    int halfHeight = region.height / 2;

    // Recursively split into 4 sections
    splitImage4N(img, regions, level - 1, cv::Rect(region.x, region.y, halfWidth, halfHeight));
    splitImage4N(img, regions, level - 1, cv::Rect(region.x + halfWidth, region.y, halfWidth, halfHeight));
    splitImage4N(img, regions, level - 1, cv::Rect(region.x, region.y + halfHeight, halfWidth, halfHeight));
    splitImage4N(img, regions, level - 1, cv::Rect(region.x + halfWidth, region.y + halfHeight, halfWidth, halfHeight));
}

// Function to apply the filter to a region
void filterRegion(const cv::Mat& img, cv::Mat& output_img, const cv::Rect& region, int choice, int kernelSize, cv::Mat& kernel, double sigmaX) {
    cv::Mat regionImg = img(region);  // Extract the region of the image

    cv::Mat filteredRegion;
    switch (choice) {
        case 1:
            filteredRegion = applyMedianFilter(regionImg, kernelSize);
            break;
        case 2:
            filteredRegion = applyLaplacianFilter(regionImg, kernelSize);
            break;
        case 3:
            filteredRegion = applyGaussianFilter(regionImg, cv::Size(kernelSize, kernelSize), sigmaX);
            break;
        case 4:
            filteredRegion = applyCustomFilter(regionImg, kernel);
            break;
        default:
            std::cerr << "Invalid filter choice!" << std::endl;
            return;
    }

    // Copy the filtered region back to the output image
    filteredRegion.copyTo(output_img(region));
}

int main() {
    // Load the image
    cv::Mat img = loadImage("image.jpg");

    // Display the original image
    displayImage("Original Image", img);

    // Ask user which mode they want to use
    int mode;
    std::cout << "Select the mode to use:" << std::endl;
    std::cout << "1. OpenCV Original Multithreading (Mode 1)" << std::endl;
    std::cout << "2. Unique 4^n Splitting (Mode 2)" << std::endl;
    std::cout << "Enter your choice (1/2): ";
    std::cin >> mode;

    // Ask user which filter they want to apply
    int choice;
    std::cout << "Select the filter to apply:" << std::endl;
    std::cout << "1. Median Filter" << std::endl;
    std::cout << "2. Laplacian Filter" << std::endl;
    std::cout << "3. Gaussian Filter" << std::endl;
    std::cout << "4. Custom Filter" << std::endl;
    std::cout << "Enter your choice (1/2/3/4): ";
    std::cin >> choice;

    int kernelSize;
    cv::Mat kernel;
    double sigmaX = 0;

    if (choice == 4) {
        std::tie(kernelSize, kernel) = loadKernel("kernel.txt");
    }
    else {
        std::cout << "Enter kernel size (odd number): ";
        std::cin >> kernelSize;
    }

    if (choice == 3) {
        std::cout << "Enter sigmaX for Gaussian filter: ";
        std::cin >> sigmaX;
    }

    // Ask user how many sections (threads) they want to divide the image into (for Mode 1)
    int numThreads = 0;
    if (mode == 1) {
        std::cout << "Enter the number of horizontal sections (threads): ";
        std::cin >> numThreads;
    }

    // Prepare the output image
    cv::Mat output_img = img.clone();

    // Set OpenCV to use only 1 thread (disabling internal parallelism for Mode 1)
    if (mode == 1) {
        cv::setNumThreads(1);
    }

    // Handle splitting for Mode 1 (OpenCV multithreading method)
    std::vector<cv::Rect> regions;
    if (mode == 1) {
        splitImageHorizontally(img, regions, numThreads);
    }

    // Handle splitting for Mode 2 (4^n unique method)
    if (mode == 2) {
        int recursionLevel;
        std::cout << "Enter the recursion level (for 4^n splitting): ";
        std::cin >> recursionLevel;
        splitImage4N(img, regions, recursionLevel, cv::Rect(0, 0, img.cols, img.rows));
    }

    // Prepare for multithreading
    std::vector<std::thread> threads;

    // Create threads to process each region
    for (const auto& region : regions) {
        threads.emplace_back(filterRegion, std::ref(img), std::ref(output_img), region, choice, kernelSize, kernel, sigmaX);
    }

    // Wait for all threads to finish
    for (auto& t : threads) t.join();

    // Display the filtered image
    displayImage("Filtered Image", output_img);

    return 0;
}
