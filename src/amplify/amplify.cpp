#include "amplify.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <tuple>
#include <map>

namespace amplify {

// Helper structure for distance transform algorithm
struct DistancePoint {
    int trace, sample;
    float distance;
    
    DistancePoint(int t, int s, float d) : trace(t), sample(s), distance(d) {}
    
    bool operator>(const DistancePoint& other) const {
        return distance > other.distance;
    }
};

FloatMask distanceTransformEDT(const BooleanMask& binary_mask, 
                               const std::vector<float>& sampling) {
    if (binary_mask.empty() || binary_mask[0].empty()) {
        return FloatMask();
    }
    
    size_t n_traces = binary_mask.size();
    size_t n_samples = binary_mask[0].size();
    
    FloatMask distance_map(n_traces, std::vector<float>(n_samples, 
                                                       std::numeric_limits<float>::infinity()));
    
    // Initialize distance map
    for (size_t i = 0; i < n_traces; ++i) {
        for (size_t j = 0; j < n_samples; ++j) {
            if (!binary_mask[i][j]) {
                distance_map[i][j] = 0.0f;
            }
        }
    }
    
    // Apply sampling rates
    float trace_sampling = sampling[0];
    float time_sampling = sampling[1];
    
    // Forward pass
    for (size_t i = 0; i < n_traces; ++i) {
        for (size_t j = 0; j < n_samples; ++j) {
            if (binary_mask[i][j]) {
                float min_dist = distance_map[i][j];
                
                // Check previous trace
                if (i > 0) {
                    float dist = distance_map[i-1][j] + trace_sampling;
                    min_dist = std::min(min_dist, dist);
                }
                
                // Check previous sample
                if (j > 0) {
                    float dist = distance_map[i][j-1] + time_sampling;
                    min_dist = std::min(min_dist, dist);
                }
                
                // Check diagonal (previous trace and sample)
                if (i > 0 && j > 0) {
                    float dist = distance_map[i-1][j-1] + 
                                std::sqrt(trace_sampling * trace_sampling + time_sampling * time_sampling);
                    min_dist = std::min(min_dist, dist);
                }
                
                distance_map[i][j] = min_dist;
            }
        }
    }
    
    // Backward pass
    for (int i = n_traces - 1; i >= 0; --i) {
        for (int j = n_samples - 1; j >= 0; --j) {
            if (binary_mask[i][j]) {
                float min_dist = distance_map[i][j];
                
                // Check next trace
                if (i < static_cast<int>(n_traces) - 1) {
                    float dist = distance_map[i+1][j] + trace_sampling;
                    min_dist = std::min(min_dist, dist);
                }
                
                // Check next sample
                if (j < static_cast<int>(n_samples) - 1) {
                    float dist = distance_map[i][j+1] + time_sampling;
                    min_dist = std::min(min_dist, dist);
                }
                
                // Check diagonal (next trace and sample)
                if (i < static_cast<int>(n_traces) - 1 && j < static_cast<int>(n_samples) - 1) {
                    float dist = distance_map[i+1][j+1] + 
                                std::sqrt(trace_sampling * trace_sampling + time_sampling * time_sampling);
                    min_dist = std::min(min_dist, dist);
                }
                
                distance_map[i][j] = min_dist;
            }
        }
    }
    
    return distance_map;
}

FloatMask createTransitionMask(
    const std::pair<size_t, size_t>& seismic_data_shape,
    const BooleanMask& window_indices,
    int transition_width_traces,
    float transition_width_time_ms,
    float dt_ms,
    TransitionMode transition_mode) {
    
    size_t n_traces = seismic_data_shape.first;
    size_t n_samples = seismic_data_shape.second;
    
    if (transition_width_traces <= 0 || transition_width_time_ms <= 0) {
        // Return window indices as float mask
        FloatMask mask(n_traces, std::vector<float>(n_samples, 0.0f));
        for (size_t i = 0; i < n_traces; ++i) {
            for (size_t j = 0; j < n_samples; ++j) {
                mask[i][j] = window_indices[i][j] ? 1.0f : 0.0f;
            }
        }
        return mask;
    }
    
    float transition_width_samples = transition_width_time_ms / dt_ms;
    std::vector<float> sampling = {1.0f / transition_width_traces, 1.0f / transition_width_samples};
    
    FloatMask mask(n_traces, std::vector<float>(n_samples, 0.0f));
    
    if (transition_mode == TransitionMode::OUTSIDE) {
        // Create inverted mask for distance transform
        BooleanMask inverted_mask = window_indices;
        for (size_t i = 0; i < inverted_mask.size(); ++i) {
            for (size_t j = 0; j < inverted_mask[i].size(); ++j) {
                inverted_mask[i][j] = !inverted_mask[i][j];
            }
        }
        
        FloatMask distances = distanceTransformEDT(inverted_mask, sampling);
        
        for (size_t i = 0; i < n_traces; ++i) {
            for (size_t j = 0; j < n_samples; ++j) {
                float transition_factor = std::max(0.0f, std::min(1.0f, 1.0f - distances[i][j]));
                mask[i][j] = window_indices[i][j] ? 1.0f : transition_factor;
            }
        }
    } else { // INSIDE
        FloatMask distances = distanceTransformEDT(window_indices, sampling);
        
        // Find maximum distance inside the window
        float max_dist_inside = 0.0f;
        for (size_t i = 0; i < n_traces; ++i) {
            for (size_t j = 0; j < n_samples; ++j) {
                if (window_indices[i][j]) {
                    max_dist_inside = std::max(max_dist_inside, distances[i][j]);
                }
            }
        }
        
        if (max_dist_inside == 0.0f) {
            // Return window indices as float mask
            for (size_t i = 0; i < n_traces; ++i) {
                for (size_t j = 0; j < n_samples; ++j) {
                    mask[i][j] = window_indices[i][j] ? 1.0f : 0.0f;
                }
            }
            return mask;
        }
        
        for (size_t i = 0; i < n_traces; ++i) {
            for (size_t j = 0; j < n_samples; ++j) {
                if (window_indices[i][j]) {
                    mask[i][j] = distances[i][j] / max_dist_inside;
                } else {
                    mask[i][j] = 0.0f;
                }
            }
        }
    }
    
    return mask;
}

BooleanMask createWindowMask(
    const std::pair<size_t, size_t>& seismic_data_shape,
    const std::vector<Point>& target_window,
    float dt_ms) {
    
    size_t n_traces = seismic_data_shape.first;
    size_t n_samples = seismic_data_shape.second;
    
    BooleanMask window_indices(n_traces, std::vector<bool>(n_samples, false));
    
    if (target_window.empty()) {
        return window_indices;
    }
    
    // For rectangle (2 points) or polygon (3+ points)
    if (target_window.size() == 2) {
        // Rectangle case - fill the rectangular area
        const Point& p1 = target_window[0];
        const Point& p2 = target_window[1];
        
        int min_trace = std::min(p1.trace, p2.trace);
        int max_trace = std::max(p1.trace, p2.trace);
        int min_sample = std::min(static_cast<int>(p1.time_ms / dt_ms), static_cast<int>(p2.time_ms / dt_ms));
        int max_sample = std::max(static_cast<int>(p1.time_ms / dt_ms), static_cast<int>(p2.time_ms / dt_ms));
        
        // Ensure valid range
        min_trace = std::max(0, std::min(static_cast<int>(n_traces) - 1, min_trace));
        max_trace = std::max(0, std::min(static_cast<int>(n_traces) - 1, max_trace));
        min_sample = std::max(0, std::min(static_cast<int>(n_samples) - 1, min_sample));
        max_sample = std::max(0, std::min(static_cast<int>(n_samples) - 1, max_sample));
        
        // Fill rectangle
        for (int trace = min_trace; trace <= max_trace; ++trace) {
            for (int sample = min_sample; sample <= max_sample; ++sample) {
                window_indices[trace][sample] = true;
            }
        }
        return window_indices;
    } else if (target_window.size() < 3) {
        // Single point case - fall back to simple point-based approach
        for (const auto& point : target_window) {
            int trace = point.trace;
            int sample = static_cast<int>(point.time_ms / dt_ms);
            
            if (trace >= 0 && trace < static_cast<int>(n_traces) && 
                sample >= 0 && sample < static_cast<int>(n_samples)) {
                window_indices[trace][sample] = true;
            }
        }
        return window_indices;
    }
    
    // Find polygon boundaries
    int min_trace = target_window[0].trace;
    int max_trace = target_window[0].trace;
    
    for (const auto& point : target_window) {
        min_trace = std::min(min_trace, point.trace);
        max_trace = std::max(max_trace, point.trace);
    }
    
    // Create closed polygon (add first point at the end)
    std::vector<Point> closed_polygon = target_window;
    closed_polygon.push_back(target_window[0]);
    
    // Rasterize polygon by traces
    for (int trace_idx = min_trace; trace_idx <= max_trace; ++trace_idx) {
        std::vector<float> intersections;
        
        // Find intersections with polygon edges
        for (size_t i = 0; i < closed_polygon.size() - 1; ++i) {
            const Point& p1 = closed_polygon[i];
            const Point& p2 = closed_polygon[i + 1];
            
            float x1 = p1.trace;
            float y1 = p1.time_ms;
            float x2 = p2.trace;
            float y2 = p2.time_ms;
            
            // Check if trace intersects this edge (handle both directions)
            if (x1 != x2) {
                float t = (trace_idx - x1) / (x2 - x1);
                if (t >= 0.0f && t <= 1.0f) {
                    float y_intersect = y1 + t * (y2 - y1);
                    intersections.push_back(y_intersect);
                }
            }
        }
        
        // Sort intersections
        std::sort(intersections.begin(), intersections.end());
        
        // Fill between pairs of intersections
        for (size_t i = 0; i < intersections.size(); i += 2) {
            if (i + 1 < intersections.size()) {
                int start_sample = static_cast<int>(intersections[i] / dt_ms);
                int end_sample = static_cast<int>(intersections[i + 1] / dt_ms);
                
                // Ensure valid range
                start_sample = std::max(0, std::min(static_cast<int>(n_samples) - 1, start_sample));
                end_sample = std::max(0, std::min(static_cast<int>(n_samples) - 1, end_sample));
                
                // Fill the range
                for (int sample = start_sample; sample <= end_sample; ++sample) {
                    if (trace_idx >= 0 && trace_idx < static_cast<int>(n_traces)) {
                        window_indices[trace_idx][sample] = true;
                    }
                }
            }
        }
    }
    
    return window_indices;
}

float calculateRMS(const SeismicData& data, const BooleanMask& mask) {
    if (data.empty() || data[0].empty()) {
        return 0.0f;
    }
    
    double sum_squares = 0.0;
    int count = 0;
    
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data[i].size(); ++j) {
            if (mask[i][j]) {
                sum_squares += static_cast<double>(data[i][j] * data[i][j]);
                ++count;
            }
        }
    }
    
    if (count == 0) {
        return 0.0f;
    }
    
    return static_cast<float>(std::sqrt(sum_squares / count));
}

std::tuple<size_t, size_t, size_t, size_t> findMaskBoundaries(const BooleanMask& mask) {
    if (mask.empty() || mask[0].empty()) {
        return {0, 0, 0, 0};
    }
    
    size_t min_trace = mask.size();
    size_t max_trace = 0;
    size_t min_sample = mask[0].size();
    size_t max_sample = 0;
    
    bool found = false;
    
    for (size_t i = 0; i < mask.size(); ++i) {
        for (size_t j = 0; j < mask[i].size(); ++j) {
            if (mask[i][j]) {
                found = true;
                min_trace = std::min(min_trace, i);
                max_trace = std::max(max_trace, i);
                min_sample = std::min(min_sample, j);
                max_sample = std::max(max_sample, j);
            }
        }
    }
    
    if (!found) {
        return {0, 0, 0, 0};
    }
    
    return {min_trace, max_trace, min_sample, max_sample};
}

AmplifyResult amplifySeismicWindow(
    const SeismicData& seismic_data,
    float dt_ms,
    const std::vector<Point>& target_window,
    ProcessingMode mode,
    float scale_factor,
    int transition_width_traces,
    float transition_width_time_ms,
    TransitionMode transition_mode,
    int align_width_traces,
    float align_width_time_ms) {
    
    if (seismic_data.empty() || seismic_data[0].empty()) {
        throw std::invalid_argument("Seismic data is empty");
    }
    
    size_t n_traces = seismic_data.size();
    size_t n_time_samples = seismic_data[0].size();
    
    AmplifyResult result(n_traces, n_time_samples);
    
    // Create binary mask for selected area
    BooleanMask window_indices = createWindowMask({n_traces, n_time_samples}, target_window, dt_ms);
    
    if (target_window.empty()) {
        result.output_data = seismic_data;
        return result;
    }
    
    // Check if any window indices are set
    bool has_window = false;
    for (const auto& row : window_indices) {
        for (bool val : row) {
            if (val) {
                has_window = true;
                break;
            }
        }
        if (has_window) break;
    }
    
    if (!has_window) {
        result.output_data = seismic_data;
        return result;
    }
    
    // Create weight mask with smooth transition
    FloatMask blending_mask = createTransitionMask(
        {n_traces, n_time_samples}, window_indices, transition_width_traces,
        transition_width_time_ms, dt_ms, transition_mode
    );
    
    // Determine target amplification coefficient
    float target_amplification = 1.0f;
    
    if (mode == ProcessingMode::SCALE) {
        target_amplification = scale_factor;
    } else if (mode == ProcessingMode::ALIGN) {
        // Calculate RMS inside window
        float rms_in_window = calculateRMS(seismic_data, window_indices);
        
        // Build surrounding area as AABB expansion (fast, like Python version)
        int align_width_time_samples = static_cast<int>(align_width_time_ms / dt_ms);
        
        // Find AABB of the window
        int min_trace = n_traces, max_trace = -1;
        int min_sample = n_time_samples, max_sample = -1;
        
        for (size_t i = 0; i < n_traces; ++i) {
            for (size_t j = 0; j < n_time_samples; ++j) {
                if (window_indices[i][j]) {
                    min_trace = std::min(min_trace, static_cast<int>(i));
                    max_trace = std::max(max_trace, static_cast<int>(i));
                    min_sample = std::min(min_sample, static_cast<int>(j));
                    max_sample = std::max(max_sample, static_cast<int>(j));
                }
            }
        }
        
        // Expand AABB by align widths
        int expanded_min_trace = std::max(0, min_trace - align_width_traces);
        int expanded_max_trace = std::min(static_cast<int>(n_traces) - 1, max_trace + align_width_traces);
        int expanded_min_sample = std::max(0, min_sample - align_width_time_samples);
        int expanded_max_sample = std::min(static_cast<int>(n_time_samples) - 1, max_sample + align_width_time_samples);
        
        // Create surrounding mask as expanded AABB minus window area
        BooleanMask surrounding_mask(n_traces, std::vector<bool>(n_time_samples, false));
        
        for (int i = expanded_min_trace; i <= expanded_max_trace; ++i) {
            for (int j = expanded_min_sample; j <= expanded_max_sample; ++j) {
                if (!window_indices[i][j]) {  // Only areas outside the window
                    surrounding_mask[i][j] = true;
                }
            }
        }
        
        float rms_surrounding;
        bool has_surrounding = false;
        for (const auto& row : surrounding_mask) {
            for (bool val : row) {
                if (val) {
                    has_surrounding = true;
                    break;
                }
            }
            if (has_surrounding) break;
        }
        
        if (has_surrounding) {
            rms_surrounding = calculateRMS(seismic_data, surrounding_mask);
        } else {
            // If surrounding area is empty, don't change anything
            rms_surrounding = rms_in_window;
        }
        
        // Avoid division by zero if window is silent
        if (rms_in_window > 1e-9f) {
            target_amplification = rms_surrounding / rms_in_window;
        } else {
            target_amplification = 1.0f;
        }
    }
    
    // Create final multiplier mask and apply
    for (size_t i = 0; i < n_traces; ++i) {
        for (size_t j = 0; j < n_time_samples; ++j) {
            result.multiplier_mask[i][j] = 1.0f + blending_mask[i][j] * (target_amplification - 1.0f);
            result.output_data[i][j] = seismic_data[i][j] * result.multiplier_mask[i][j];
        }
    }
    
    result.window_indices = window_indices;
    
    return result;
}

} // namespace amplify
