#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

class ParallelImageProcesser {
public:
    // Pad all sides of the image with 0's
    static vector<vector<int>> addPadding(vector<vector<int>> &input, int padding) {

        int rows = input.size();
        int cols = input[0].size();

        cout << "rows: " << rows << ", cols: " << cols << endl;
        vector<vector<int>> res(rows + padding*2, vector<int>(cols + padding*2, 0));

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                res[i+padding][j+padding] = input[i][j];
            }
        }
        return res;
    }

    // This is the sequential version
    static vector<vector<int>> sequentialConvolve(vector<vector<int>> &input, vector<vector<int>> &filter, int stride = 1) {

        int image_rows = input.size();
        int image_cols = input[0].size();
        int filter_rows = filter.size();
        int filter_cols = filter[0].size();
        int res_rows = ((image_rows - filter_rows) / stride) + 1;
        int res_cols = ((image_cols - filter_cols) / stride) + 1;

        vector<vector<int>> res(res_rows, vector<int>(res_cols, 0));

        for (int i = 0; i < res_rows; i++) {
            for (int j = 0; j < res_cols; j++) {
                res[i][j] = applyFilter(ref(input), i*stride, j*stride, ref(filter));
            }
        }

        return res;
    }

    // This is the parallel version
    static vector<vector<int>> parallelConvolve(vector<vector<int>> &input, vector<vector<int>> &filter, int stride = 1) {

        // Intermediate variables to make things easier to work with
        int image_rows = input.size();
        int image_cols = input[0].size();
        int filter_rows = filter.size();
        int filter_cols = filter[0].size();
        int res_rows = ((image_rows - filter_rows) / stride) + 1;
        int res_cols = ((image_cols - filter_cols) / stride) + 1;

        vector<vector<int>> res(res_rows, vector<int>(res_cols, 0));

        // Right now thread count is hard coded.
        // It must be a number with an integer square root.
        // Not checking if number of rows and cols in result is less than sqrt of num_threads.
        vector<thread> threads;
        int num_threads = 4;
        int sqrt_num_threads = static_cast<int>(sqrt(num_threads));

        // How many rows and columns each thread need to handle at most
        int chunk_rows = (res_rows % sqrt_num_threads == 0) ? (res_rows / sqrt_num_threads) : (res_rows / sqrt_num_threads + 1);
        int chunk_cols = (res_cols % sqrt_num_threads == 0) ? (res_cols / sqrt_num_threads) : (res_cols / sqrt_num_threads + 1);

        // Starting the threads
        for (int i = 0; i < sqrt_num_threads; i++) {
            for (int j = 0; j < sqrt_num_threads; j++) {
                threads.emplace_back(parallelConvWorker, ref(input), ref(filter), ref(res), stride, i*chunk_rows, i*chunk_rows+chunk_rows, j*chunk_cols, j*chunk_cols+chunk_cols);
            }
        }

        for (auto& t : threads) {
            t.join();
        }

        return res;
    }

private:
    // This is what happens during thread work.
    // Accepts, the input, filter, and result arrays, start row, end row, start col, and end col for the thread.
    static void parallelConvWorker(vector<vector<int>> &input, vector<vector<int>> &filter, vector<vector<int>> &res, int stride, int start_row, int end_row, int start_col, int end_col) {
        
        // Since last thread might not have the full chunk
        end_row = min(end_row, static_cast<int>(res.size()));
        end_col = min(end_col, static_cast<int>(res[0].size()));

        // Make convolution for each result slot
        for (int i = start_row; i < end_row; i++) {
            for (int j = start_col; j < end_col; j++) {
                res[i][j] = applyFilter(ref(input), i*stride, j*stride, ref(filter));
            }
        }
    }

    // The convoultion
    static int applyFilter(vector<vector<int>> &input, int top, int left, vector<vector<int>> &filter) {
        int res = 0;
        for (int i = 0; i < filter.size(); i++) {
            for (int j = 0; j < filter[0].size(); j++) {
                res += input[i+top][j+left] * filter[i][j];
            }
        }
        return res;
    }
};

void printImage(vector<vector<int>> &image) {
    int rows = image.size();
    int cols = image[0].size();
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << setw(3) << image[i][j] << " ";
        }
        cout << endl;
    }
}

int main() {

    vector<vector<int>> image = {
        {1, 2, 3, 4, 2},
        {5, 6 ,7, 8, 4},
        {9, 4, 1, 9, 1},
        {1, 4, 5, 2, 3},
        {1, 2, 3, 4, 2}
    };

    vector<vector<int>> filter = {
        {1, 2},
        {3, 4},
    };

    image = ParallelImageProcesser::addPadding(ref(image), 1);
    printImage(image);

    vector<vector<int>> res1 = ParallelImageProcesser::sequentialConvolve(ref(image), ref(filter), 3);
    printImage(res1);

    vector<vector<int>> res2 = ParallelImageProcesser::parallelConvolve(ref(image), ref(filter), 3);
    printImage(res2);

    // Test with random numbers
    vector<vector<int>> image2(30, vector<int>(40));
    srand(static_cast<unsigned int>(std::time(0)));

    for (int i = 0; i < image2.size(); i++) {
        for (int j = 0; j < image2[0].size(); j++) {
            image2[i][j] = rand() % 10;
        }
    }

    image2 = ParallelImageProcesser::addPadding(ref(image2), 1);
    printImage(image2);
    vector<vector<int>> res3 = ParallelImageProcesser::sequentialConvolve(ref(image2), ref(filter), 4);
    printImage(res3);
    cout << endl;
    vector<vector<int>> res4 = ParallelImageProcesser::parallelConvolve(ref(image2), ref(filter), 4);
    printImage(res4);


    return 0;
}