#ifndef SEGY_WRITER_H
#define SEGY_WRITER_H

#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <stdexcept>

namespace ioutils { 

/**
 * @brief Class for writing SEGY files
 * 
 * This class writes SEGY files from trace data and metadata.
 * It can use a reference SEGY file to copy headers and structure.
 */
class SegyWriter {
public:
    /**
     * @brief Constructor
     * @param target_path Path where the new SEGY file will be written
     * @param reference_path Path to reference SEGY file (for copying headers)
     * @throws std::runtime_error if reference file cannot be opened
     */
    SegyWriter(const std::string& target_path, const std::string& reference_path);
    
    /**
     * @brief Destructor
     */
    ~SegyWriter() = default;
    
    // Disable copy constructor and assignment operator
    SegyWriter(const SegyWriter&) = delete;
    SegyWriter& operator=(const SegyWriter&) = delete;
    
    /**
     * @brief Write SEGY file with new trace data
     * @param data 2D vector containing trace data [trace][sample]
     * @param sample_interval Sample interval in seconds
     * @throws std::runtime_error if writing fails
     */
    void writeFile(const std::vector<std::vector<float>>& data, double sample_interval);
    
    /**
     * @brief Write SEGY file with new trace data and custom headers
     * @param data 2D vector containing trace data [trace][sample]
     * @param sample_interval Sample interval in seconds
     * @param trace_headers 2D vector containing trace headers [trace][240 bytes]
     * @throws std::runtime_error if writing fails
     */
    void writeFile(const std::vector<std::vector<float>>& data, 
                   double sample_interval,
                   const std::vector<std::vector<char>>& trace_headers);

private:
    std::string target_path_;
    std::string reference_path_;
    
    // Reference file data
    std::vector<char> text_header_;      // 3200 bytes
    std::vector<char> binary_header_;    // 400 bytes
    std::vector<std::vector<char>> reference_trace_headers_;  // Trace headers from reference
    
    // Helper functions
    uint16_t swapBytes16(uint16_t val) const;
    uint32_t swapBytes32(uint32_t val) const;
    uint32_t ieeeToIbm(float f) const;
    void readReferenceFile();
    void writeTextHeader(std::ofstream& file) const;
    void writeBinaryHeader(std::ofstream& file, double sample_interval, size_t num_samples) const;
    void writeTraces(std::ofstream& file, 
                     const std::vector<std::vector<float>>& data,
                     const std::vector<std::vector<char>>& trace_headers) const;
};
} // namespace ioutils
#endif // SEGY_WRITER_H
