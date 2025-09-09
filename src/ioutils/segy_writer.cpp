#include "segy_writer.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace ioutils { 

SegyWriter::SegyWriter(const std::string& target_path, const std::string& reference_path)
    : target_path_(target_path), reference_path_(reference_path) {
    readReferenceFile();
}

void SegyWriter::readReferenceFile() {
    std::ifstream file(reference_path_, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open reference SEGY file: " + reference_path_);
    }
    
    // Read text header (3200 bytes)
    text_header_.resize(3200);
    file.read(text_header_.data(), 3200);
    if (file.gcount() != 3200) {
        throw std::runtime_error("Failed to read text header from reference file");
    }
    
    // Read binary header (400 bytes)
    binary_header_.resize(400);
    file.read(binary_header_.data(), 400);
    if (file.gcount() != 400) {
        throw std::runtime_error("Failed to read binary header from reference file");
    }
    
    // Read all trace headers
    const size_t trace_header_size = 240;
    
    // First, get the number of samples per trace from binary header
    uint16_t n_samples_per_trace;
    std::memcpy(&n_samples_per_trace, binary_header_.data() + 20, sizeof(n_samples_per_trace));
    n_samples_per_trace = swapBytes16(n_samples_per_trace);
    
    if (n_samples_per_trace == 0) {
        throw std::runtime_error("Number of samples per trace is zero in reference file");
    }
    
    const size_t full_trace_size = trace_header_size + n_samples_per_trace * sizeof(uint32_t);
    
    // Count traces
    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    file.seekg(3600); // Start of traces
    
    // Fix ambiguity by explicitly casting to streamoff
    std::streamoff data_size = file_size - std::streamoff(3600);
    size_t num_traces = static_cast<size_t>(data_size / full_trace_size);
    
    // Read all trace headers
    reference_trace_headers_.resize(num_traces);
    for (size_t i = 0; i < num_traces; ++i) {
        reference_trace_headers_[i].resize(trace_header_size);
        file.read(reference_trace_headers_[i].data(), trace_header_size);
        
        if (file.gcount() != trace_header_size) {
            throw std::runtime_error("Failed to read trace header " + std::to_string(i) + 
                                   " from reference file");
        }
        
        // Skip trace data
        file.seekg(n_samples_per_trace * sizeof(uint32_t), std::ios::cur);
    }
    
    file.close();
}

void SegyWriter::writeFile(const std::vector<std::vector<float>>& data, double sample_interval) {
    // Use reference trace headers
    writeFile(data, sample_interval, reference_trace_headers_);
}

void SegyWriter::writeFile(const std::vector<std::vector<float>>& data, 
                           double sample_interval,
                           const std::vector<std::vector<char>>& trace_headers) {
    if (data.empty()) {
        throw std::runtime_error("Data is empty");
    }
    
    size_t num_traces = data.size();
    size_t num_samples = data[0].size();
    
    // Validate that all traces have the same number of samples
    for (size_t i = 1; i < num_traces; ++i) {
        if (data[i].size() != num_samples) {
            throw std::runtime_error("All traces must have the same number of samples");
        }
    }
    
    // Validate trace headers
    if (trace_headers.size() != num_traces) {
        throw std::runtime_error("Number of trace headers must match number of traces");
    }
    
    for (size_t i = 0; i < num_traces; ++i) {
        if (trace_headers[i].size() != 240) {
            throw std::runtime_error("Each trace header must be exactly 240 bytes");
        }
    }
    
    std::ofstream file(target_path_, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create output SEGY file: " + target_path_);
    }
    
    // Write text header
    writeTextHeader(file);
    
    // Write binary header
    writeBinaryHeader(file, sample_interval, num_samples);
    
    // Write traces
    writeTraces(file, data, trace_headers);
    
    file.close();
}

void SegyWriter::writeTextHeader(std::ofstream& file) const {
    file.write(text_header_.data(), text_header_.size());
    if (!file.good()) {
        throw std::runtime_error("Failed to write text header");
    }
}

void SegyWriter::writeBinaryHeader(std::ofstream& file, double sample_interval, size_t num_samples) const {
    // Copy reference binary header
    std::vector<char> header_copy = binary_header_;
    
    // Update sample interval (offset 3216, 2 bytes)
    uint16_t dt_us = static_cast<uint16_t>(sample_interval * 1e6);
    dt_us = swapBytes16(dt_us);
    std::memcpy(header_copy.data() + 16, &dt_us, sizeof(dt_us));
    
    // Update number of samples per trace (offset 3220, 2 bytes)
    uint16_t n_samples = static_cast<uint16_t>(num_samples);
    n_samples = swapBytes16(n_samples);
    std::memcpy(header_copy.data() + 20, &n_samples, sizeof(n_samples));
    
    file.write(header_copy.data(), header_copy.size());
    if (!file.good()) {
        throw std::runtime_error("Failed to write binary header");
    }
}

void SegyWriter::writeTraces(std::ofstream& file, 
                             const std::vector<std::vector<float>>& data,
                             const std::vector<std::vector<char>>& trace_headers) const {
    for (size_t i = 0; i < data.size(); ++i) {
        // Write trace header
        file.write(trace_headers[i].data(), trace_headers[i].size());
        if (!file.good()) {
            throw std::runtime_error("Failed to write trace header " + std::to_string(i));
        }
        
        // Write trace data
        for (size_t j = 0; j < data[i].size(); ++j) {
            uint32_t ibm = ieeeToIbm(data[i][j]);
            ibm = swapBytes32(ibm);
            file.write(reinterpret_cast<const char*>(&ibm), sizeof(ibm));
            
            if (!file.good()) {
                throw std::runtime_error("Failed to write sample " + std::to_string(j) + 
                                       " from trace " + std::to_string(i));
            }
        }
    }
}

uint16_t SegyWriter::swapBytes16(uint16_t val) const {
    return (val << 8) | (val >> 8);
}

uint32_t SegyWriter::swapBytes32(uint32_t val) const {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

uint32_t SegyWriter::ieeeToIbm(float f) const {
    uint32_t ieee;
    std::memcpy(&ieee, &f, sizeof(uint32_t));
    
    if (ieee == 0) return 0;
    
    static const int it[4] = { 0x21200000, 0x21400000, 0x21800000, 0x22100000 };
    static const int mt[4] = { 2, 4, 8, 1 };
    
    int ix = (ieee & 0x01800000) >> 23;
    uint32_t iexp = ((ieee & 0x7e000000) >> 1) + it[ix];
    uint32_t manthi = (mt[ix] * (ieee & 0x007fffff)) >> 3;
    uint32_t ibm_bits = (manthi + iexp) | (ieee & 0x80000000);
    
    return (ieee & 0x7fffffff) ? ibm_bits : 0;
}

}
