#include <fstream>
#include <iostream>

#include <vector>
#include <complex>

#include <memory>
#include <cmath>
#include <chrono>

#include <thread>
#include <mutex>
#include <functional>

#include <png.h>

struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

const int RADIUS = 2;
const int MAX_ITERATIONS = 1000;

const double cxMin = -1.35;
const double cxMax = .75;

const double cyMin = -1.25;
const double cyMax = 1.25;

const int WIDTH = 1920;
const int HEIGHT = 1080;

std::mutex m;

void colorPixel(Pixel& p, int iterations) {
    double iterationPercentage = 1.0 * iterations / MAX_ITERATIONS;

    unsigned char value = static_cast<unsigned char>(std::floor(iterationPercentage * 255.0));

    p = { 0, value, value };
}

void renderRow(
    int &rowIndex,
    volatile int &lastRowWritten,
    png_structp png_ptr
    ) {
    // Mandlebrot equation
    // Zn+1 = Zn^2 + c
    // Z and C are complex numbers and n is zero or positive integer

    std::unique_ptr<Pixel[]> row (new Pixel[WIDTH]);

    while (rowIndex < HEIGHT) {
        std::unique_lock<std::mutex> lock(m);

        int y = rowIndex++;
        lock.unlock();

        if (y == HEIGHT) {
            break;
        }

        for (int x = 0; x < WIDTH; ++x) {
            double scaledX = (x / (WIDTH - 1.0) * (cxMax - cxMin)) + cxMin;
            double scaledY = (y / (HEIGHT - 1.0) * (cyMax - cyMin)) + cyMin;

            std::complex<double> c(scaledX, scaledY);
            std::complex<double> z = 0;
            
            int iter = 0;

            for (iter; iter < MAX_ITERATIONS && std::abs(z) <= RADIUS; ++iter) {
                z = std::pow(z, 2) + c;
            }

            colorPixel(row[x], iter);
        }

        while (y != lastRowWritten) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        lock.lock();
        png_write_row(png_ptr, reinterpret_cast<const unsigned char*>(row.get()));
        ++lastRowWritten;
    }
}

int main() {
    auto start = std::chrono::system_clock::now();

    std::ofstream output("mandlebrot.png", std::ios::binary);
    output.close();

    FILE *fp = fopen("mandlebrot.png", "wb");

    png_structp png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        NULL,
        NULL,
        NULL
    );

    if (!png_ptr) {
        std::cerr << "png_create_write failed";
        abort();
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        std::cerr << "png_create_info failed";
        abort();
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "error during writing header";
        abort();
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(
        png_ptr,
        info_ptr,
        WIDTH,
        HEIGHT,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );

    png_write_info(png_ptr, info_ptr);

    int row = 0;
    volatile int lastRowWritten = 0;

    std::vector<std::thread> threads;

    for (int i = 0; i < 20; ++i) {
        threads.emplace_back(
            renderRow,
            std::ref(row),
            std::ref(lastRowWritten),
            png_ptr);
    }

    for (auto &thread: threads) {
        thread.join();
    }

    png_write_end(png_ptr, NULL);
    fclose(fp);

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << diff.count() << " seconds" << std::endl;

    return 0;
}