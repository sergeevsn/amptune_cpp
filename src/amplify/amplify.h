#ifndef AMPLIFY_H
#define AMPLIFY_H

#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <stdexcept>

/**
 * @brief Namespace for seismic data amplification and alignment functions
 */
namespace amplify {

/**
 * @brief Point structure for window coordinates
 */
struct Point {
    int trace;
    float time_ms;
    
    Point(int t, float s) : trace(t), time_ms(s) {}
    Point() : trace(0), time_ms(0.0f) {}
};

/**
 * @brief 2D matrix type for seismic data
 */
using SeismicData = std::vector<std::vector<float>>;

/**
 * @brief 2D boolean mask type
 */
using BooleanMask = std::vector<std::vector<bool>>;

/**
 * @brief 2D float mask type
 */
using FloatMask = std::vector<std::vector<float>>;

/**
 * @brief Result structure for amplification operations
 */
struct AmplifyResult {
    SeismicData output_data;      // Processed seismic data
    FloatMask multiplier_mask;    // Applied multiplier mask
    BooleanMask window_indices;   // Window selection mask
    
    AmplifyResult(size_t n_traces, size_t n_samples) 
        : output_data(n_traces, std::vector<float>(n_samples, 0.0f)),
          multiplier_mask(n_traces, std::vector<float>(n_samples, 1.0f)),
          window_indices(n_traces, std::vector<bool>(n_samples, false)) {}
};

/**
 * @brief Transition mode enumeration
 */
enum class TransitionMode {
    OUTSIDE,  // Transition from outside the window
    INSIDE    // Transition from inside the window
};

/**
 * @brief Processing mode enumeration
 */
enum class ProcessingMode {
    SCALE,    // Scale amplitudes by factor
    ALIGN     // Align amplitudes with surrounding area
};

/**
 * @brief Euclidean Distance Transform implementation
 * 
 * Computes the distance transform of a binary image using the Euclidean distance.
 * This is a C++ implementation of scipy.ndimage.distance_transform_edt
 * 
 * @param binary_mask Input binary mask (true = object, false = background)
 * @param sampling Sampling rates for each dimension [trace_sampling, time_sampling]
 * @return Distance map where each pixel contains the distance to the nearest background pixel
 */
FloatMask distanceTransformEDT(const BooleanMask& binary_mask, 
                               const std::vector<float>& sampling);

/**
 * @brief Creates a weight mask for smooth amplification transitions
 * 
 * Creates a weight mask (from 0.0 to 1.0) for smooth amplification.
 * This is the C++ equivalent of create_transition_mask from amplify.py
 * 
 * @param seismic_data_shape Shape of the seismic data (n_traces, n_samples)
 * @param window_indices Binary mask indicating the selected window
 * @param transition_width_traces Width of transition zone in traces
 * @param transition_width_time_ms Width of transition zone in milliseconds
 * @param dt_ms Sample interval in milliseconds
 * @param transition_mode Transition mode (OUTSIDE or INSIDE)
 * @return Weight mask with smooth transitions
 */
FloatMask createTransitionMask(
    const std::pair<size_t, size_t>& seismic_data_shape,
    const BooleanMask& window_indices,
    int transition_width_traces,
    float transition_width_time_ms,
    float dt_ms,
    TransitionMode transition_mode = TransitionMode::OUTSIDE
);

/**
 * @brief Amplifies or aligns seismic data amplitudes in the specified window
 * 
 * This is the main function for seismic data amplification and alignment.
 * This is the C++ equivalent of amplify_seismic_window from amplify.py
 * 
 * @param seismic_data Input seismic data as 2D vector
 * @param dt_ms Sample interval in milliseconds
 * @param target_window List of points defining the target window
 * @param mode Processing mode (SCALE or ALIGN)
 * @param scale_factor Scale factor for SCALE mode (default: 1.0)
 * @param transition_width_traces Width of transition zone in traces (default: 5)
 * @param transition_width_time_ms Width of transition zone in milliseconds (default: 20.0)
 * @param transition_mode Transition mode (default: INSIDE)
 * @param align_width_traces Width for alignment in traces (default: 10)
 * @param align_width_time_ms Width for alignment in milliseconds (default: 50.0)
 * @return AmplifyResult containing processed data and masks
 */
AmplifyResult amplifySeismicWindow(
    const SeismicData& seismic_data,
    float dt_ms,
    const std::vector<Point>& target_window,
    ProcessingMode mode,
    float scale_factor = 1.0f,
    int transition_width_traces = 5,
    float transition_width_time_ms = 20.0f,
    TransitionMode transition_mode = TransitionMode::INSIDE,
    int align_width_traces = 10,
    float align_width_time_ms = 50.0f
);

/**
 * @brief Helper function to calculate RMS (Root Mean Square) of data in a mask
 * 
 * @param data Input data
 * @param mask Boolean mask indicating which values to include
 * @return RMS value
 */
float calculateRMS(const SeismicData& data, const BooleanMask& mask);

/**
 * @brief Helper function to find boundaries of a boolean mask
 * 
 * @param mask Input boolean mask
 * @return Tuple of (min_trace, max_trace, min_sample, max_sample)
 */
std::tuple<size_t, size_t, size_t, size_t> findMaskBoundaries(const BooleanMask& mask);

/**
 * @brief Helper function to create a window mask from point coordinates
 * 
 * @param seismic_data_shape Shape of the seismic data
 * @param target_window List of points defining the window
 * @param dt_ms Sample interval in milliseconds
 * @return Boolean mask indicating the selected window
 */
BooleanMask createWindowMask(
    const std::pair<size_t, size_t>& seismic_data_shape,
    const std::vector<Point>& target_window,
    float dt_ms
);

} // namespace amplify

#endif // AMPLIFY_H
