// segy_reader.h
#ifndef SEGY_READER_H
#define SEGY_READER_H

#include <vector>
#include <string>
#include <fstream> // <- Убедитесь, что fstream подключен здесь
#include <cstdint>
#include <stdexcept>

namespace ioutils { 

/**
 * @brief Class for reading SEGY files
 * 
 * This class reads SEGY files and provides access to trace data and metadata.
 * It reads the entire file into memory during construction for efficient access.
 */
class SegyReader {
public:
    /**
     * @brief Constructor that reads the SEGY file
     * @param file_path Path to the SEGY file
     * @throws std::runtime_error if file cannot be opened or read
     */
    explicit SegyReader(const std::string& file_path);
    
    /**
     * @brief Destructor
     */
    ~SegyReader() = default;
    
    // Disable copy constructor and assignment operator
    SegyReader(const SegyReader&) = delete;
    SegyReader& operator=(const SegyReader&) = delete;
    
    /**
     * @brief Get the number of traces in the file
     * @return Number of traces
     */
    size_t getNumTraces() const { return num_traces_; }
    
    /**
     * @brief Get the number of samples per trace
     * @return Number of samples per trace
     */
    size_t getNumSamples() const { return num_samples_; }
    
    /**
     * @brief Get the sample interval in seconds
     * @return Sample interval in seconds
     */
    double getDt() const { return dt_; }
    
    /**
     * @brief Get a specific trace by index
     * @param trace_index Index of the trace (0-based)
     * @return Vector containing the trace data
     * @throws std::out_of_range if trace_index is invalid
     */
    const std::vector<float>& getTrace(size_t trace_index) const;
    
    /**
     * @brief Get all traces as a 2D vector
     * @return 2D vector where each inner vector is a trace
     */
    const std::vector<std::vector<float>>& getAllTraces() const { return traces_; }
    
    /**
     * @brief Get a specific trace header by index
     * @param trace_index Index of the trace (0-based)
     * @return Vector containing the trace header (240 bytes)
     * @throws std::out_of_range if trace_index is invalid
     */
    const std::vector<char>& getTraceHeader(size_t trace_index) const;
    
    /**
     * @brief Get the binary header
     * @return Vector containing the binary header (400 bytes)
     */
    const std::vector<char>& getBinaryHeader() const { return binary_header_; }

private:
    std::string file_path_;
    size_t num_traces_;
    size_t num_samples_;
    double dt_;  // Sample interval in seconds
    
    std::vector<std::vector<float>> traces_;  // 2D vector: [trace][sample]
    std::vector<std::vector<char>> trace_headers_;  // Trace headers
    std::vector<char> binary_header_;  // Binary header (400 bytes)
    
    // Helper functions
    uint16_t swapBytes16(uint16_t val) const;
    uint32_t swapBytes32(uint32_t val) const;
    float ibmToIeee(uint32_t ibm) const;
    
    // --- ИЗМЕНЕНИЯ ЗДЕСЬ ---
    // Объявления функций теперь должны принимать файловый поток.
    // Функция readFile() больше не нужна, так как ее логика в конструкторе.
    void readBinaryHeader(std::ifstream& file);
    void readTraces(std::ifstream& file);
};

} // namespace ioutils

#endif // SEGY_READER_H