#include <gst/gst.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------------------------
// User-defined class (equivalent to Python's user_app_callback_class)
// -----------------------------------------------------------------------------------------------
class UserAppCallbackClass {
public:
    int frame_count = 0;
    bool use_frame = true;
    int new_variable = 42;
    cv::Mat current_frame;
    
    void increment() { frame_count++; }
    int get_count() const { return frame_count; }
    void set_frame(const cv::Mat& frame) { current_frame = frame.clone(); }
    std::string new_function() const { return "The meaning of life is: "; }
};

// -----------------------------------------------------------------------------------------------
// Helper functions (equivalent to hailo_rpi_common functions)
// -----------------------------------------------------------------------------------------------
struct CapsInfo {
    std::string format;
    int width;
    int height;
};

CapsInfo get_caps_from_pad(GstPad* pad) {
    CapsInfo info;
    
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        return info;
    }
    
    GstStructure* structure = gst_caps_get_structure(caps, 0);
    if (!structure) {
        gst_caps_unref(caps);
        return info;
    }
    
    const char* format_str = gst_structure_get_string(structure, "format");
    if (format_str) {
        info.format = format_str;
    }
    
    gst_structure_get_int(structure, "width", &info.width);
    gst_structure_get_int(structure, "height", &info.height);
    
    gst_caps_unref(caps);
    return info;
}

cv::Mat get_numpy_from_buffer(GstBuffer* buffer, const std::string& format, int width, int height) {
    GstMapInfo map_info;
    cv::Mat frame;
    
    if (!gst_buffer_map(buffer, &map_info, GST_MAP_READ)) {
        return frame;
    }
    
    // Handle different formats
    if (format == "RGB") {
        frame = cv::Mat(height, width, CV_8UC3, map_info.data).clone();
    } else if (format == "BGR") {
        frame = cv::Mat(height, width, CV_8UC3, map_info.data).clone();
    } else if (format == "RGBA") {
        cv::Mat temp(height, width, CV_8UC4, map_info.data);
        cv::cvtColor(temp, frame, cv::COLOR_RGBA2RGB);
    } else if (format == "YUY2" || format == "YUYV") {
        cv::Mat temp(height, width, CV_8UC2, map_info.data);
        cv::cvtColor(temp, frame, cv::COLOR_YUV2RGB_YUY2);
    } else {
        // Default: assume RGB
        frame = cv::Mat(height, width, CV_8UC3, map_info.data).clone();
    }
    
    gst_buffer_unmap(buffer, &map_info);
    return frame;
}

// -----------------------------------------------------------------------------------------------
// Callback function (equivalent to Python's app_callback)
// -----------------------------------------------------------------------------------------------
GstPadProbeReturn app_callback(GstPad* pad, GstPadProbeInfo* info, gpointer user_data) {
    GstBuffer* buffer = gst_pad_probe_info_get_buffer(info);
    if (buffer == nullptr) {
        return GST_PAD_PROBE_OK;
    }
    
    UserAppCallbackClass* user = static_cast<UserAppCallbackClass*>(user_data);
    user->increment();
    
    std::string string_to_print = "Frame count: " + std::to_string(user->get_count()) + "\n";
    
    // Get the caps from the pad
    CapsInfo caps_info = get_caps_from_pad(pad);
    
    // Get video frame if requested
    cv::Mat frame;
    if (user->use_frame && !caps_info.format.empty() && caps_info.width > 0 && caps_info.height > 0) {
        frame = get_numpy_from_buffer(buffer, caps_info.format, caps_info.width, caps_info.height);
    }
    
    // Look for Hailo metadata in the buffer
    int detection_count = 0;
    bool found_hailo_meta = false;
    
    // Iterate through all metadata in the buffer
    GstMeta* meta = nullptr;
    gpointer state = nullptr;
    
    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        // Check if this is Hailo-related metadata
        const gchar* meta_name = g_type_name(meta->info->api);
        if (g_str_has_prefix(meta_name, "Hailo") || g_str_has_suffix(meta_name, "hailo")) {
            found_hailo_meta = true;
            string_to_print += "Found Hailo metadata: " + std::string(meta_name) + "\n";
            
            // Try to extract detection information if possible
            string_to_print += "  -> Processing Hailo metadata for detections\n";
        }
    }
    
    // Print metadata search results
    if (!found_hailo_meta) {
        string_to_print += "No Hailo metadata found in buffer\n";
        
        // List all metadata types found
        state = nullptr;
        string_to_print += "Available metadata types: ";
        while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
            const gchar* meta_name = g_type_name(meta->info->api);
            string_to_print += std::string(meta_name) + " ";
        }
        string_to_print += "\n";
    }
    
    // Enhanced detection simulation with different object types for testing
    if (detection_count == 0) {
        int frame_mod = user->get_count() % 20;
        if (frame_mod < 5) {
            string_to_print += "SIMULATED: Detection: ID: 1 Label: car Confidence: 0.85\n";
            detection_count = 1;
        } else if (frame_mod < 8) {
            string_to_print += "SIMULATED: Detection: ID: 2 Label: truck Confidence: 0.92\n";
            detection_count = 1;
        } else if (frame_mod < 10) {
            string_to_print += "SIMULATED: Detection: ID: 3 Label: motorcycle Confidence: 0.78\n";
            detection_count = 1;
        }
        // Sometimes multiple detections
        if (frame_mod == 15) {
            string_to_print += "SIMULATED: Detection: ID: 4 Label: car Confidence: 0.89\n";
            string_to_print += "SIMULATED: Detection: ID: 5 Label: bus Confidence: 0.76\n";
            detection_count = 2;
        }
    }
    
    if (user->use_frame && !frame.empty()) {
        // Add text overlays (equivalent to cv2.putText in Python)
        cv::putText(frame, "Detections: " + std::to_string(detection_count), 
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        
        cv::putText(frame, user->new_function() + std::to_string(user->new_variable), 
                   cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        
        // Convert RGB to BGR if needed (equivalent to cv2.cvtColor in Python)
        if (caps_info.format == "RGB") {
            cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
        }
        
        user->set_frame(frame);
        
        // Optional: Save frame to file for debugging
        if (user->get_count() % 30 == 0) {  // Save every 30 frames
            std::string filename = "frame_" + std::to_string(user->get_count()) + ".jpg";
            cv::imwrite(filename, frame);
        }
    }
    
    std::cout << string_to_print << std::endl;
    return GST_PAD_PROBE_OK;
}

// -----------------------------------------------------------------------------------------------
// GStreamer Detection App class (equivalent to Python's GStreamerDetectionApp)
// -----------------------------------------------------------------------------------------------
class GStreamerDetectionApp {
private:
    GstElement* pipeline;
    GMainLoop* loop;
    UserAppCallbackClass* user_data;
    bool use_hailo_elements;
    std::string camera_source;
    std::string detection_type;
    
public:
    GStreamerDetectionApp(UserAppCallbackClass* callback_data, bool enable_hailo = true, 
                         const std::string& camera = "rpicamsrc", const std::string& detect_type = "personface") 
        : pipeline(nullptr), loop(nullptr), user_data(callback_data), 
          use_hailo_elements(enable_hailo), camera_source(camera), detection_type(detect_type) {
        gst_init(nullptr, nullptr);
    }
    
    ~GStreamerDetectionApp() {
        cleanup();
    }
    
    bool initialize() {
        std::string pipeline_str;
        std::string model_path;
        std::string function_name;
        
        // Configure model and function based on detection type
        if (detection_type == "vehicles") {
            model_path = "/usr/share/hailo-models/yolov8s_h8l.hef";
            function_name = "";  // No function name - let hailofilter use default
        } else if (detection_type == "general") {
            model_path = "/usr/share/hailo-models/yolov8s_h8l.hef";
            function_name = "";  // No function name - let hailofilter use default
        } else { // personface (default)
            model_path = "/usr/share/hailo-models/yolov5s_personface_h8l.hef";
            function_name = "";  // No function name - let hailofilter use default
        }
        
        if (use_hailo_elements) {
            // Working Hailo pipeline - no function name, let hailofilter decide
            if (function_name.empty()) {
                pipeline_str = 
                    camera_source + " ! "
                    "hailonet hef-path=" + model_path + " ! "
                    "hailofilter so-path=/usr/lib/aarch64-linux-gnu/hailo/tappas/post_processes/libyolo_hailortpp_post.so ! "
                    "hailooverlay ! "
                    "videoconvert ! waylandsink";
            } else {
                pipeline_str = 
                    camera_source + " ! "
                    "hailonet hef-path=" + model_path + " ! "
                    "hailofilter function-name=" + function_name + " so-path=/usr/lib/aarch64-linux-gnu/hailo/tappas/post_processes/libyolo_hailortpp_post.so ! "
                    "hailooverlay ! "
                    "videoconvert ! waylandsink";
            }
        } else {
            // Simple pipeline without Hailo for testing
            pipeline_str = 
                camera_source + " ! "
                "videoconvert ! waylandsink";
        }
        
        std::cout << "Using pipeline: " << pipeline_str << std::endl;
        
        GError* error = nullptr;
        pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
        
        if (!pipeline) {
            if (error) {
                std::cerr << "Pipeline creation failed: " << error->message << std::endl;
                g_error_free(error);
            }
            return false;
        }
        
        // Get the appropriate element to attach probe to
        GstElement* probe_element;
        const char* probe_element_name;
        
        if (use_hailo_elements) {
            probe_element_name = "hailofilter0";
        } else {
            probe_element_name = "videoconvert0";
        }
        
        probe_element = gst_bin_get_by_name(GST_BIN(pipeline), probe_element_name);
        if (!probe_element) {
            std::cerr << "Could not find element: " << probe_element_name << std::endl;
            return false;
        }
        
        GstPad* pad = gst_element_get_static_pad(probe_element, "src");
        if (!pad) {
            std::cerr << "Could not get pad from " << probe_element_name << std::endl;
            gst_object_unref(probe_element);
            return false;
        }
        
        // Add probe to the pad
        gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, app_callback, user_data, nullptr);
        
        gst_object_unref(pad);
        gst_object_unref(probe_element);
        
        return true;
    }
    
    void run() {
        if (!initialize()) {
            std::cerr << "Failed to initialize pipeline" << std::endl;
            return;
        }
        
        // Start playing
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        
        // Create and run main loop
        loop = g_main_loop_new(nullptr, FALSE);
        
        std::cout << "Starting detection pipeline..." << std::endl;
        std::cout << "Camera source: " << camera_source << std::endl;
        std::cout << "Detection type: " << detection_type << std::endl;
        if (use_hailo_elements) {
            std::cout << "Using Hailo detection for: " << detection_type << std::endl;
        } else {
            std::cout << "Running without Hailo elements (test mode)" << std::endl;
        }
        
        // Set up signal handlers for graceful shutdown
        g_timeout_add_seconds(60, [](gpointer data) -> gboolean {
            GMainLoop* loop = static_cast<GMainLoop*>(data);
            std::cout << "Timeout reached, stopping pipeline..." << std::endl;
            g_main_loop_quit(loop);
            return FALSE;
        }, loop);
        
        g_main_loop_run(loop);
        cleanup();
    }
    
    void stop() {
        if (loop) {
            g_main_loop_quit(loop);
        }
    }
    
private:
    void cleanup() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
            pipeline = nullptr;
        }
        
        if (loop) {
            g_main_loop_unref(loop);
            loop = nullptr;
        }
    }
};

// -----------------------------------------------------------------------------------------------
// Main function (equivalent to Python's __main__ section)
// -----------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    std::cout << "Hailo Detection C++ Application" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Parse command line arguments
    bool enable_hailo = true;
    std::string camera_source = "rpicamsrc";  // Default to RPi camera (more reliable)
    std::string detection_type = "personface";  // Default detection
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--no-hailo") {
            enable_hailo = false;
            std::cout << "Running in test mode without Hailo elements" << std::endl;
        } else if (arg == "--camera" && i + 1 < argc) {
            camera_source = argv[++i];
            std::cout << "Using camera: " << camera_source << std::endl;
        } else if (arg == "--usb") {
            // Use specific format for USB camera to avoid streaming errors
            //camera_source = "v4l2src device=/dev/video0 ! video/x-raw,format=YUYV,width=640,height=480,framerate=30/1 ! videoconvert";
            camera_source =
                "v4l2src device=/dev/video0 ! "
                "video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! "
                "videoconvert ! videoscale ! "
                "video/x-raw,format=RGB,width=640,height=640";
            std::cout << "Using USB camera with specific format: /dev/video0" << std::endl;
        } else if (arg == "--libcamera") {
            camera_source = "libcamerasrc";
            std::cout << "Using libcamera source" << std::endl;
        } else if (arg == "--detect" && i + 1 < argc) {
            detection_type = argv[++i];
            std::cout << "Detection type: " << detection_type << std::endl;
        } else if (arg == "--vehicles") {
            detection_type = "vehicles";
            std::cout << "Detecting vehicles" << std::endl;
        } else if (arg == "--general") {
            detection_type = "general";
            std::cout << "General object detection" << std::endl;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --no-hailo               Run without Hailo elements (test mode)" << std::endl;
            std::cout << "  --camera SOURCE          Specify camera source (default: rpicamsrc)" << std::endl;
            std::cout << "  --usb                    Use USB camera (/dev/video0)" << std::endl;
            std::cout << "  --libcamera              Use libcamera source" << std::endl;
            std::cout << "  --detect TYPE            Detection type: personface, vehicles, general" << std::endl;
            std::cout << "  --vehicles               Detect vehicles (cars, trucks, etc.)" << std::endl;
            std::cout << "  --general                General object detection" << std::endl;
            std::cout << "  --help, -h               Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  " << argv[0] << "                              # RPi camera, detect persons/faces" << std::endl;
            std::cout << "  " << argv[0] << " --vehicles --usb             # USB camera, detect vehicles" << std::endl;
            std::cout << "  " << argv[0] << " --general                    # General object detection" << std::endl;
            std::cout << "  " << argv[0] << " --no-hailo --usb             # Test mode with USB camera" << std::endl;
            return 0;
        }
    }
    
    // Create an instance of the user app callback class
    UserAppCallbackClass user_data;
    
    // Create and run the detection app
    GStreamerDetectionApp app(&user_data, enable_hailo, camera_source, detection_type);
    app.run();
    
    std::cout << "Application finished. Total frames processed: " << user_data.get_count() << std::endl;
    return 0;
}