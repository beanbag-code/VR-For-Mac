#define GL_SILENCE_DEPRECATION
#include <opencv2/opencv.hpp>
#include <iostream>
#include "apriltag/apriltag.h"
#include "apriltag/tag36h11.h"
#include "apriltag/apriltag_pose.h"
#include <opencv2/core/quaternion.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <thread>
#include <mutex>
#include <atomic>
#include "external/ImGui/imgui.h"
#include "external/ImGui/imgui_impl_glfw.h"
#include "external/ImGui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::vector<std::string> port_list;

struct EulerAngles{
    double pitch, roll, yaw;
};

struct StoredQuats
{
    cv::Quatd imu;
};
StoredQuats ForwardConst{cv::Quatd(1, 0, 0, 0)};

struct CorrectedQuats
{
    cv::Quatd returnedData;
};

//CorrectedQuats CorrectQuat(cv::Quatd tagDat, cv::Quatd imuDat, double speed)
CorrectedQuats CorrectQuat(cv::Quatd imuDat)
{
    CorrectedQuats quat;
    quat.returnedData = ForwardConst.imu.inv() * imuDat;
    return quat;
}


EulerAngles ToEuler(cv::Quatd q)
{
    EulerAngles angles;

    double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
    double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
    angles.roll = std::atan2(sinr_cosp, cosr_cosp);

    double sinp = 2 * (q.w * q.y - q.z * q.x);
    
    if (sinp > 1.0) sinp = 1.0;
    if (sinp < -1.0) sinp = -1.0;

    if (std::abs(sinp) >= 0.999)
        angles.pitch = std::copysign(M_PI / 2, sinp);
    else
        angles.pitch = std::asin(sinp);

    double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
    double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
    angles.yaw = std::atan2(siny_cosp, cosy_cosp);

    angles.pitch *= (180.0 / M_PI);
    angles.roll  *= (180.0 / M_PI);
    angles.yaw   *= (180.0 / M_PI);

    return angles;
}




#pragma pack(push, 1)
struct DREF_Packet {
    char header[5] = {'D', 'R', 'E', 'F', 0};
    float value;
    char path[500]; // Total packet size must be 509 bytes
};
#pragma pack(pop)

void SetDataRef(int sock, struct sockaddr_in6& addr, const std::string& path, float val) {
    DREF_Packet pkt;
    pkt.value = val;
    
    // Completely zero out the 500-byte buffer first
    memset(pkt.path, 0, 500); 
    
    // Copy the string, leaving room for at least one null at the end
    if (path.length() < 500) {
        memcpy(pkt.path, path.c_str(), path.length());
    }

    sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr*)&addr, sizeof(addr));
}

void RefreshSerialPorts(){
    port_list.clear();
    DIR *dir = opendir("/dev");
    if(dir == NULL)
    {
        std::cerr << "Could not open /dev directory" << std::endl;
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if(name.find("tty.usb") != std::string::npos ||
            name.find("tty.cu") != std::string::npos)
            {
                port_list.push_back("/dev/" + name);
            }
    } 
    closedir(dir);

}

struct IMUData {
    cv::Quatd imuQuat = cv::Quatd(1,0,0,0);
};
IMUData globalIMU;
std::mutex imu_mutex;
std::atomic<bool> keep_running(true);

void serialWorker();

std::atomic<bool> serialConnected{false};
char serialPath[128] = "/dev/tty.usbmodem1101";

int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    //sets constants for tags
    //float tagsize = .2;
    //float fx = 1000.0;
    //float fy = 1000.0;
    //float cx = 640.0;
    //float cy = 360.0;

    //cv::Quatd qTag = cv::Quatd(1,0,0,0);
    //double predictionCertianty = 1.0;
    //bool tagDetected = false;

    // Un-commented these state variables so the main loop compiles and executes properly
    bool ImGuiRunning = true;
    bool calabrating = false;

    //Gets GLFW running
    glfwInit();
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    //Sets MAC helpers
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    //Creates the GLFW window
    GLFWwindow* window = glfwCreateWindow(int(1280 * main_scale), (int)(800 *main_scale), "DebugWindow", nullptr, nullptr);
    if(window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    
    // OPTIMIZATION: Set VSync to 0 (disabled) to run loop uncapped at maximum speed
    glfwSwapInterval(0);

    //Starts ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    //sets the camera up
    //cv::VideoCapture cap(0);
    //if (!cap.isOpened()) {
    //    std::cerr << "Error: Could not open camera." << std::endl;
    //    return -1;
    //}

    //creates the april tag decetor
    //apriltag_family_t * tf = tag36h11_create();
    //apriltag_detector_t * td = apriltag_detector_create();
    //apriltag_detector_add_family(td, tf);

    //td->quad_decimate = 2.0;
    //td->nthreads = 4;

    //GLuint texture_id;
    
    //Starts the GPU slot
    //glGenTextures(1, &texture_id);
    //glBindTexture(GL_TEXTURE_2D, texture_id);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    RefreshSerialPorts();


    int xp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if(xp_socket < 0){
        std::cerr <<"Could not open UDP" << std::endl;
    }

    struct sockaddr_in6 xp_addr;
    memset(&xp_addr, 0, sizeof(xp_addr));
    xp_addr.sin6_family = AF_INET6;
    xp_addr.sin6_port = htons(49000);
    inet_pton(AF_INET6, "::ffff:127.0.0.1", &xp_addr.sin6_addr);

    //main loop
        while (ImGuiRunning)
        {
            glfwPollEvents();

            //if(cap.grab())
            //{
                //cv::Mat frame;
                //cap.retrieve(frame);
                //if (frame.empty()) break;
                
                //cv::Mat gray;
                //cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                
                //image_u8_t img = 
                //{ 
                 //   .width = gray.cols,
                //    .height = gray.rows,
                //    .stride = gray.cols,
                //    .buf = gray.data 
                //};

                //cv::Mat textureFrame;
                //cv::cvtColor(gray, textureFrame, cv::COLOR_GRAY2RGBA);

                //glBindTexture(GL_TEXTURE_2D, texture_id);
               // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureFrame.cols, textureFrame.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureFrame.data);


                //zarray_t *detections = apriltag_detector_detect(td, &img);

                //for (int i = 0; i < zarray_size(detections); i++)
                //{
                //    apriltag_detection_t *det;
                //   zarray_get(detections, i, &det);

                  //  apriltag_detection_info_t info;
                    //info.det = det;
                  //  info.tagsize = tagsize;
                //    info.fx = fx;
                  //  info.fy = fy;
                  //  info.cx = cx;
                  //  info.cy = cy;

                  //  apriltag_pose_t pose;
                  //  double err = estimate_tag_pose(&info, &pose);
                  //  if (pose.R && pose.R->nrows == 3 && pose.R->ncols == 3) {
                  //      cv::Mat rotMat(3, 3, CV_64F, pose.R->data);
                   //     try {
                   //         cv::Quatd candidate = cv::Quatd::createFromRotMat(rotMat);
                    //        qTag = candidate;
                     //       predictionCertianty = det->decision_margin;
                    //        tagDetected = true;
                    //        std::cout << "Detected Tag Quat: " << qTag << std::endl;
                    //    } catch (const cv::Exception& e) {
                    //        std::cerr << "Skipping invalid tag pose: " << e.what() << std::endl;
                   //     }
                  //  } else {
                  //      std::cerr << "Skipping invalid pose matrix from AprilTag detection" << std::endl;
                 //   }

                 //   matd_destroy(pose.R);
               //     matd_destroy(pose.t);
                //    
               // }
                //if(detections)
               // {
                    //Clean up
               //     apriltag_detections_destroy(detections);
                //}

           // }

            IMUData current_imu;
            {
                std::lock_guard<std::mutex> lock(imu_mutex);
                current_imu = globalIMU;
            }
            
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            static int selectedPortIdx = 0;
            if(!serialConnected)
            {
                ImGui::Begin("Serial Conect");

                if(ImGui::Button("Refresh Ports"))
                {
                    RefreshSerialPorts();
                }

                const char* preview = (port_list.empty()) ? "No Devices Found" : port_list[selectedPortIdx].c_str();

                if(ImGui::BeginCombo("Select IMU Port", preview))
                {
                    for (int i = 0; i < port_list.size(); i++)
                    {
                        bool isSelected = (selectedPortIdx == i);
                        if (ImGui::Selectable(port_list[i].c_str(), isSelected))
                        {
                            selectedPortIdx = i;
                            memset(serialPath, 0, sizeof(serialPath));
                            strncpy(serialPath, port_list[i].c_str(), sizeof(serialPath) - 1);
                        }
                    }
                    ImGui::EndCombo();
                }
                if(!port_list.empty() && ImGui::Button("Connect")) {
                    keep_running = true;
                    // Start the worker thread and let it set connected state on success.
                    std::thread(serialWorker).detach();
                }
                ImGui::End();
            }  else
            {
                if(!calabrating){

                    ImGui::Begin("Debug Window");
                   // ImGui::Image((void*)(intptr_t)texture_id, ImVec2(640, 480));
                    ImGui::Text("Hello World");
                    //ImGui::Text("Tag Quaternion: W:%.3f X:%.3f Y:%.3f Z:%.3f", qTag.w, qTag.x, qTag.y, qTag.z);
                    ImGui::Text("IMU Quaternion: W:%.3f X:%.3f Y:%.3f Z:%.3f", current_imu.imuQuat.w, current_imu.imuQuat.x, current_imu.imuQuat.y, current_imu.imuQuat.z);
                    //ImGui::Text("Confidence: C:%.3f", predictionCertianty);
                    if (ImGui::Button("Calabrate"))
                    {
                        calabrating = true;
                    }
                    cv::Quatd fusedQ = cv::Quatd(1,0,0,0);

                    // FIXED: Added missing semicolon and mapped return struct to .returnedData
                    fusedQ = CorrectQuat(current_imu.imuQuat).returnedData;
                    
                   // double qtw = std::abs(qTag.w);
                   // if (qtw > 1.0) qtw = 1.0;
                    //double angleRadians = std::acos(qtw);
                    //if (tagDetected && 2 * angleRadians < 35 && predictionCertianty < 0.01)
                   // {
                      //  double certintySpeed = (angleRadians / predictionCertianty);
                     //   fusedQ = CorrectQuat(qTag, current_imu.imuQuat, 0.01).returnedData;
                   // }
                   // else
                   // {
                   //     fusedQ = (ForwardConst.imu.inv() * current_imu.imuQuat);
                   // }
    
                    
                    if (serialConnected) 
                    {
                        EulerAngles orientation = ToEuler(fusedQ);

                        // OPTIMIZATION: Removed the artificial 60Hz limit block entirely 
                        // so updates blast out directly as fast as the uncapped loop ticks.
                        // static double lastSendTime = 0;
                        // double currentTime = glfwGetTime();
                        // if (currentTime - lastSendTime > (1.0 / 60.0)) {
                            SetDataRef(xp_socket, xp_addr, "sim/operation/override/override_view_head", 1.0f);
    
                            SetDataRef(xp_socket, xp_addr, "sim/graphics/view/pilots_head_the", ((float)orientation.roll * -1));
                            
                            SetDataRef(xp_socket, xp_addr, "sim/graphics/view/pilots_head_psi", (float)orientation.pitch);
                            
                            SetDataRef(xp_socket, xp_addr, "sim/graphics/view/pilots_head_phi", (float)orientation.yaw * -1);
                            
                            // lastSendTime = currentTime;
                        // }
                    }
    
                    if(ImGui::Button("Disconnect")) {
                        keep_running = false; 
                        serialConnected = false;
                        
                        std::lock_guard<std::mutex> lock(imu_mutex);
                        globalIMU.imuQuat = {1.0, 0.0, 0.0, 0.0};
                        // qTag = cv::Quatd(1, 0, 0, 0); // Commented out since AprilTag variable is disabled
                    }
                    ImGui::End();
                } else
                {
                    static bool calComplete = false;
                    ImGui::Begin("Debug Window");
                    ImGui::Text("Look Straight Forward");
                    if(ImGui::Button("Start"))
                    {
                        ForwardConst.imu = cv::Quatd(current_imu.imuQuat.w, current_imu.imuQuat.x, current_imu.imuQuat.y, current_imu.imuQuat.z);
                        //ForwardConst.aprilTag = qTag;
                        calComplete = true;
                    }
                    if(calComplete)
                    {
                        if(ImGui::Button("Return to Debug"))
                        {
                            calabrating = false;
                            calComplete = false;
                        }
                    }
                    ImGui::End();
                }
            }


            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
                
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) 
            {
                ImGuiRunning = false;
            }
        }
    keep_running = false;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //glfwHideWindow(window);
    //glfwDestroyWindow(window);

  //  apriltag_detector_destroy(td);
   // tag36h11_destroy(tf);
    return 0;
}

void serialWorker()
{
    int serial_port = open(serialPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_port < 0) {
        serialConnected = false;
        return;
    }

    serialConnected = true;

    // Set Baud Rate (B115200)
    struct termios tty;
    tcgetattr(serial_port, &tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tcsetattr(serial_port, TCSANOW, &tty);

    std::string accumulator = "";
    char read_buff[256];

    while (keep_running) {
        int n = read(serial_port, read_buff, sizeof(read_buff) - 1);
        if (n > 0) {
            read_buff[n] = '\0'; 
            accumulator += read_buff;

            // HIGH PERFORMANCE SERIAL OPTIMIZATION: 
            // Instead of looping through and updating globalIMU 50 times for backlogged 
            // text streams, look for the *last* complete newline packet instantly, 
            // process that, and drop the older historic lines to eliminate lag.
            size_t last_newline = accumulator.rfind('\n');
            if (last_newline != std::string::npos) {
                size_t prev_newline = accumulator.rfind('\n', last_newline - 1);
                size_t start_pos = (prev_newline == std::string::npos) ? 0 : prev_newline + 1;
                std::string line = accumulator.substr(start_pos, last_newline - start_pos);
                
                accumulator.erase(0, last_newline + 1);

                double tw, tx, ty, tz;
                if (sscanf(line.c_str(), "%lf,%lf,%lf,%lf", &tw, &tx, &ty, &tz) == 4) {
                    std::lock_guard<std::mutex> lock(imu_mutex);
                    globalIMU.imuQuat = {tw, tx, ty, tz};
                }
            }

            /* --- Original Loop kept for your reference ---
            size_t newline_pos;
            while ((newline_pos = accumulator.find('\n')) != std::string::npos) {
                std::string line = accumulator.substr(0, newline_pos);
                accumulator.erase(0, newline_pos + 1);

                double tw, tx, ty, tz;
                if (sscanf(line.c_str(), "%lf,%lf,%lf,%lf", &tw, &tx, &ty, &tz) == 4) {
                    std::lock_guard<std::mutex> lock(imu_mutex);
                    globalIMU.imuQuat = {tw, tx, ty, tz};
                }
            }
            ----------------------------------------------- */
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    close(serial_port);
}