#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>
#include <chrono>
#include <windows.h>
#include <psapi.h>

namespace fs = std::filesystem;

// Filter functions
cv::Mat applyMedianFilter(const cv::Mat& img, int kernelSize) {
    cv::Mat filtered_img;
    cv::medianBlur(img, filtered_img, kernelSize);
    return filtered_img;
}

cv::Mat applyLaplacianFilter(const cv::Mat& img, int kernelSize) {
    cv::Mat filtered_img;
    cv::Laplacian(img, filtered_img, CV_8U, kernelSize);
    return filtered_img;
}

cv::Mat applyGaussianFilter(const cv::Mat& img, const cv::Size& kernelSize, double sigmaX) {
    cv::Mat filtered_img;
    cv::GaussianBlur(img, filtered_img, kernelSize, sigmaX);
    return filtered_img;
}

// Splitting functions
void splitImageHorizontally(const cv::Mat& img, std::vector<cv::Rect>& regions, int numThreads) {
    int stepY = img.rows / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        int y = i * stepY;
        int height = (i == numThreads - 1) ? img.rows - y : stepY;
        regions.push_back(cv::Rect(0, y, img.cols, height));
    }
}

void splitImage4N(const cv::Mat& img, std::vector<cv::Rect>& regions, int level, const cv::Rect& region) {
    if (level == 0) {
        regions.push_back(region);
        return;
    }
    int halfWidth = region.width / 2;
    int halfHeight = region.height / 2;
    splitImage4N(img, regions, level - 1, {region.x, region.y, halfWidth, halfHeight});
    splitImage4N(img, regions, level - 1, {region.x + halfWidth, region.y, halfWidth, halfHeight});
    splitImage4N(img, regions, level - 1, {region.x, region.y + halfHeight, halfWidth, halfHeight});
    splitImage4N(img, regions, level - 1, {region.x + halfWidth, region.y + halfHeight, halfWidth, halfHeight});
}

// Filtering function for a region
void filterRegion(const cv::Mat& img, cv::Mat& output_img, const cv::Rect& region, int choice, int kernelSize, double sigmaX) {
    cv::Mat regionImg = img(region);
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
        default:
            std::cerr << "Invalid filter choice!" << std::endl;
            return;
    }
    filteredRegion.copyTo(output_img(region));
}

// Get memory usage in KB
void getMemoryUsage(SIZE_T& memoryUsageKB) {
    PROCESS_MEMORY_COUNTERS memInfo;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo))) {
        memoryUsageKB = memInfo.WorkingSetSize / 1024;
    }
}

// Calculate a rough CPU usage estimate (%)
double calculateCPUUsage() {
    FILETIME idleTime, kernelTime, userTime;
    FILETIME procCreation, procExit, procKernel, procUser;

    if (GetSystemTimes(&idleTime, &kernelTime, &userTime) &&
        GetProcessTimes(GetCurrentProcess(), &procCreation, &procExit, &procKernel, &procUser)) {

        ULONGLONG sysTime = ((ULONGLONG)kernelTime.dwLowDateTime | ((ULONGLONG)kernelTime.dwHighDateTime << 32)) +
                            ((ULONGLONG)userTime.dwLowDateTime | ((ULONGLONG)userTime.dwHighDateTime << 32));

        ULONGLONG procTime = ((ULONGLONG)procKernel.dwLowDateTime | ((ULONGLONG)procKernel.dwHighDateTime << 32)) +
                             ((ULONGLONG)procUser.dwLowDateTime | ((ULONGLONG)procUser.dwHighDateTime << 32));

        if (sysTime > 0)
            return (procTime * 100.0) / sysTime;
    }
    return 0.0;
}

int main() {
    // ===== Configuration =====
    std::string folder = "hard"; // Set to "easy" or "hard"
    int mode = 1;                // 1 for OpenCV horizontal split, 2 for unique 4^n splitting
    int filterChoice = 3;        // 1 = Median, 2 = Laplacian, 3 = Gaussian
    int kernelSize = 11;
    double sigmaX = 5.0;         // Only used for Gaussian filter
    int numThreads = 64;          // For Mode 1 (horizontal splitting)
    int recursionLevel = 3;      // For Mode 2 (4^n splitting)

    // Set output folder and report file name based on folder and mode
    std::string outputFolder = folder + "applied";
    std::string modeName = (mode == 1) ? "opencv" : "newmethod";
    std::string reportFileName = "report(" + modeName + ")(" + folder + ").txt";

    // Create output folder if it doesn't exist
    fs::create_directory(outputFolder);

    // Open report file
    std::ofstream report(reportFileName);
    if (!report.is_open()) {
        std::cerr << "Error: Could not open report file.\n";
        return -1;
    }

    // Write report header with settings used
    report << "===== FILTER SETTINGS =====\n";
    report << "Mode: " << modeName << "\n";
    report << "Folder: " << folder << "\n";
    report << "Filter: ";
    if (filterChoice == 1)
        report << "Median Filter\n";
    else if (filterChoice == 2)
        report << "Laplacian Filter\n";
    else if (filterChoice == 3)
        report << "Gaussian Filter\n";
    report << "Kernel Size: " << kernelSize << "\n";
    if (filterChoice == 3)
        report << "SigmaX: " << sigmaX << "\n";
    if (mode == 1)
        report << "Threads: " << numThreads << "\n";
    else
        report << "Recursion Level: " << recursionLevel << "\n";
    report << "\n===== IMAGE RESULTS =====\n";

    double totalTime = 0;
    SIZE_T totalMem = 0;
    double totalCPU = 0;
    int count = 0;

    // Iterate over each image in the input folder
    for (const auto& entry : fs::directory_iterator(folder)) {
        std::string imagePath = entry.path().string();
        cv::Mat img = cv::imread(imagePath);
        if (img.empty()) {
            std::cerr << "Failed to load: " << imagePath << std::endl;
            continue;
        }

        cv::Mat output = img.clone();
        std::vector<cv::Rect> regions;
        if (mode == 1) {
            splitImageHorizontally(img, regions, numThreads);
            cv::setNumThreads(1);  // disable OpenCV internal multithreading
        } else {
            splitImage4N(img, regions, recursionLevel, cv::Rect(0, 0, img.cols, img.rows));
        }

        // Capture memory usage before processing
        SIZE_T memBefore;
        getMemoryUsage(memBefore);

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (const auto& region : regions) {
            threads.emplace_back(filterRegion, std::ref(img), std::ref(output), region, filterChoice, kernelSize, sigmaX);
        }
        for (auto& t : threads)
            t.join();

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();

        // Capture memory usage after processing
        SIZE_T memAfter;
        getMemoryUsage(memAfter);

        double cpuUsage = calculateCPUUsage();

        std::string fileName = entry.path().filename().string();
        report << fileName << ": Time = " << duration << " ms, Memory = " 
               << memAfter << " KB, CPU Usage = " << cpuUsage << " %\n";

        // Save the filtered image in the appropriate folder
        cv::imwrite(outputFolder + "/" + fileName, output);

        totalTime += duration;
        totalMem += memAfter;
        totalCPU += cpuUsage;
        count++;
    }

    if (count > 0) {
        report << "\n===== AVERAGE METRICS =====\n";
        report << "Average Time     : " << totalTime / count << " ms\n";
        report << "Average Memory   : " << totalMem / count << " KB\n";
        report << "Average CPU Usage: " << totalCPU / count << " %\n";
    }

    report.close();
    std::cout << "Processing complete. Report saved to: " << reportFileName << "\n";
    return 0;
}
