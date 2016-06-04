#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <numeric>

#include <tbb/tbb.h>
#include <cstring>

class StatsCount {
public:
    StatsCount(const std::string& filename, const size_t bufferSize = 512) : 
        fileName(filename), BUFFER_SIZE(bufferSize), result(256, 0) { }

    void run() {
        // Thread Local storage for characters counts
        tbb::combinable<std::vector<size_t>> nbChars([](){return std::vector<size_t>(256, 0);});
        tbb::combinable<std::ifstream*> files([this](){
            return new std::ifstream(fileName, std::ios::binary | std::ios::in);
        });

        // Get the file size
        std::ifstream file (fileName, std::ios::ate | std::ios::binary | std::ios::in);
        if (!file.is_open()) {
            std::cerr << "Can not open file : " << fileName << std::endl;
            return;
        }
        const size_t FILE_SIZE = (size_t)file.tellg();
        std::cout << "File size is " << FILE_SIZE << std::endl;

        // Calculate the nomber of iteration required
        const size_t NB_ITERATIONS = FILE_SIZE / BUFFER_SIZE + 
            ((FILE_SIZE % BUFFER_SIZE) != 0 ? 1 : 0);


        // Process the counting algo in parallel
        tbb::parallel_for(tbb::blocked_range<size_t>(0, NB_ITERATIONS), 
            [&nbChars, &files, this](const tbb::blocked_range<size_t> &r) {
                std::ifstream* file = files.local();
                if (!file || !file->is_open()) {
                    return;
                }

                // Place the read cursor to the good position
                file->seekg(r.begin() * BUFFER_SIZE, std::ios::beg);
                
                // Get a ref to the localstorage for this thread
                std::vector<size_t>& chars = nbChars.local();

                // Count all characters
                uint8_t* buffer = new uint8_t[BUFFER_SIZE];

                for (size_t i = r.begin(); i != r.end(); ++i) {
                    std::streamsize readed = file->readsome((char*)buffer, BUFFER_SIZE);
                    while(readed--) {
                        chars[buffer[readed]]++;
                    }
                }

                delete[] buffer;
            }
        );
        
        // Combine the result of all thread in the result 
        nbChars.combine_each([this](const std::vector<size_t> &v){
            for (size_t i = 0; i < result.size(); ++i) {
                result[i] += v[i];
            }
        });

        // Close all opened files
        files.combine_each([this](std::ifstream* file){
            file->close();
            delete file;
        });
    }

    void print() {
        for (size_t i = 0; i <= result.size(); ++i) {     
            if (isprint((int)i)) {
                std::cout << "'" << (char)i << "' : " << result[i] << std::endl;
            }
        }
        
        size_t total = std::accumulate(result.begin(), result.end(), 0);
        std::cout << std::endl << "Total char in file : " << total << std::endl;
    }

private:
    const std::string fileName;
    const size_t BUFFER_SIZE;
    std::vector<size_t> result;
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << argv[0] << " filename" << std::endl;
        return -1;
    }
    StatsCount stats(argv[1]);
    stats.run(); 
    stats.print();
    return 0;
}

