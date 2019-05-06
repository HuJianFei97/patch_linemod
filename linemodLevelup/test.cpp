#include "linemodLevelup.h"
#include <memory>
#include <iostream>
#include <assert.h>
#include <chrono>  // for high_resolution_clock
#include <opencv2/rgbd.hpp>
#include <opencv2/dnn.hpp>
#include "pose_renderer.h"
#include "pose_tree.h"

#include "Open3D/Core/Registration/Registration.h"
#include "Open3D/Core/Geometry/Image.h"
#include "Open3D/Core/Camera/PinholeCameraIntrinsic.h"
#include "Open3D/Core/Geometry/PointCloud.h"
#include "Open3D/Visualization/Visualization.h"

using namespace std;
using namespace cv;

static std::string prefix = "/home/meiqua/patch_linemod/linemodLevelup/test/case1/";
// for test
std::string type2str(int type) {
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}
static cv::Mat eulerAnglesToRotationMatrix(cv::Vec3f theta)
{
    // Calculate rotation about x axis
    cv::Mat R_x = (cv::Mat_<float>(3,3) <<
               1,       0,              0,
               0,       std::cos(theta[0]),   -std::sin(theta[0]),
               0,       std::sin(theta[0]),   std::cos(theta[0])
               );
    // Calculate rotation about y axis
    cv::Mat R_y = (cv::Mat_<float>(3,3) <<
               std::cos(theta[1]),    0,      std::sin(theta[1]),
               0,               1,      0,
               -std::sin(theta[1]),   0,      std::cos(theta[1])
               );
    // Calculate rotation about z axis
    cv::Mat R_z = (cv::Mat_<float>(3,3) <<
               std::cos(theta[2]),    -std::sin(theta[2]),      0,
               std::sin(theta[2]),    std::cos(theta[2]),       0,
               0,               0,                  1);
    // Combined rotation matrix
    cv::Mat R = R_z * R_y * R_x;
    return R;
}

void train_test(){
    Mat rgb = cv::imread("/home/meiqua/6DPose/linemodLevelup/test/869/rgb.png");
    Mat depth = cv::imread("/home/meiqua/6DPose/linemodLevelup/test/869/depth.png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);

    std::cout << type2str(depth.type());

    auto view_dep = [](cv::Mat dep){
        cv::Mat map = dep;
        double min;
        double max;
        cv::minMaxIdx(map, &min, &max);
        cv::Mat adjMap;
        map.convertTo(adjMap,CV_8UC1, 255 / (max-min), -min);
        cv::Mat falseColorsMap;
        applyColorMap(adjMap, falseColorsMap, cv::COLORMAP_HOT);
        return falseColorsMap;
    };

    imshow("depth", view_dep(depth));
    cv::waitKey(0);

    vector<Mat> sources;
    sources.push_back(rgb);
    sources.push_back(depth);
    auto detector = linemodLevelup::Detector(16, {4, 8}, 16);
    detector.addTemplate(sources, "06_template", depth>0);
//    detector.writeClasses(prefix+"writeClasses/%s.yaml");
    cout << "break point line: train_test" << endl;
}

void detect_test(){
    // test case1
    /*
     * (x=327, y=127, float similarity=92.66, class_id=06_template, template_id=424)
     * render K R t:
  cam_K: [572.41140000, 0.00000000, 325.26110000, 0.00000000, 573.57043000, 242.04899000, 0.00000000, 0.00000000, 1.00000000]
  cam_R_w2c: [0.34768538, 0.93761126, 0.00000000, 0.70540612, -0.26157897, -0.65877056, -0.61767070, 0.22904489, -0.75234390]
  cam_t_w2c: [0.00000000, 0.00000000, 1000.00000000]

  gt K R t:
- cam_R_m2c: [0.09506610, 0.98330897, -0.15512900, 0.74159598, -0.17391300, -0.64791101, -0.66407597, -0.05344890, -0.74575198]
  cam_t_m2c: [71.62781422, -158.20064191, 1050.77777823]
  obj_bb: [331, 130, 65, 64]
  obj_id: 6
  */

    Mat rgb = cv::imread(prefix+"0000_rgb.png");
    Mat rgb_half = cv::imread(prefix+"0000_rgb_half.png");
    Mat depth = cv::imread(prefix+"0000_dep.png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
    Mat depth_half = cv::imread(prefix+"0000_dep_half.png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
    vector<Mat> sources, src_half;
    sources.push_back(rgb); src_half.push_back(rgb_half);
    sources.push_back(depth); src_half.push_back(depth_half);


//    rectangle(rgb, Point(326,132), Point(347, 197), Scalar(255, 255, 255), -1);
//    rectangle(depth, Point(326,132), Point(347, 197), Scalar(255, 255, 255), -1);
//    imwrite(prefix+"0000_dep_half.png", depth);
//    imwrite(prefix+"0000_rgb_half.png", rgb);
//    imshow("rgb", rgb);
//    waitKey(0);

    Mat K_ren = (Mat_<float>(3,3) << 572.4114, 0.0, 325.2611, 0.0, 573.57043, 242.04899, 0.0, 0.0, 1.0);
    Mat R_ren = (Mat_<float>(3,3) << 0.34768538, 0.93761126, 0.00000000, 0.70540612,
                 -0.26157897, -0.65877056, -0.61767070, 0.22904489, -0.75234390);
    Mat t_ren = (Mat_<float>(3,1) << 0.0, 0.0, 1000.0);

    vector<string> classes;
    classes.push_back("06_template");

    auto detector = linemodLevelup::Detector(20,{4, 8});
    detector.readClasses(classes, prefix + "%s.yaml");

    auto start_time = std::chrono::high_resolution_clock::now();
    vector<linemodLevelup::Match> matches = detector.match(sources, 65, 0.6f, classes);
    auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
    cout << "match time: " << elapsed_time.count()/1000000000.0 <<"s" << endl;

    vector<Rect> boxes;
    vector<float> scores;
    vector<int> idxs;
    for(auto match: matches){
        Rect box;
        box.x = match.x;
        box.y = match.y;
        box.width = 40;
        box.height = 40;
        boxes.push_back(box);
        scores.push_back(match.similarity);
    }
    cv::dnn::NMSBoxes(boxes, scores, 0, 0.4, idxs);

    Mat draw = rgb;
    for(auto idx : idxs){
        auto match = matches[idx];
        int r = 40;
        cout << "x: " << match.x << "\ty: " << match.y << "\tid: " << match.template_id
             << "\tsimilarity: "<< match.similarity <<endl;
        cv::circle(draw, cv::Point(match.x+r,match.y+r), r, cv::Scalar(255, 0 ,255), 2);
        cv::putText(draw, to_string(int(round(match.similarity))),
                    Point(match.x+r-10, match.y-3), FONT_HERSHEY_PLAIN, 1.4, Scalar(0,255,255));

    }
    imshow("rgb", draw);
//    imwrite(prefix+"result/depth600_hist.png", draw);
    waitKey(0);
}

void dataset_test(){
    string pre = "/home/meiqua/6DPose/public/datasets/hinterstoisser/test/06/";
    int i=0;
    for(;i<1000;i++){
        auto i_str = to_string(i);
        for(int pad=4-i_str.size();pad>0;pad--){
            i_str = '0'+i_str;
        }
        Mat rgb = cv::imread(pre+"rgb/"+i_str+".png");
        Mat depth = cv::imread(pre+"depth/"+i_str+".png", CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);
        vector<Mat> sources;
        sources.push_back(rgb);
        sources.push_back(depth);
        auto detector = linemodLevelup::Detector(16, {4, 8});

        std::vector<int> dep_anchors = {346, 415, 498, 598, 718, 861, 1034, 1240, 1489};
        int dep_range = 200;
        vector<string> classes;
        for(int dep: dep_anchors){
            classes.push_back("01_template_"+std::to_string(dep));
        }
        detector.readClasses(classes, "/home/meiqua/6DPose/public/datasets/hinterstoisser/linemod_render_up/%s.yaml");

        auto start_time = std::chrono::high_resolution_clock::now();
        vector<linemodLevelup::Match> matches = detector.match(sources, 70, 0.6f, classes, dep_anchors, dep_range);
        auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
        cout << "match time: " << elapsed_time.count()/1000000000.0 <<"s" << endl;

        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<int> idxs;
        for(auto match: matches){
            Rect box;
            box.x = match.x;
            box.y = match.y;
            box.width = 40;
            box.height = 40;
            boxes.push_back(box);
            scores.push_back(match.similarity);
        }
        cv::dnn::NMSBoxes(boxes, scores, 0, 0.4, idxs);

        cv::Mat draw = rgb;
        for(auto idx : idxs){
            auto match = matches[idx];

//            auto templ = detector.getTemplates(match.class_id, match.template_id);
//            cv::Mat show_templ = cv::Mat(templ[0].height+1, templ[0].width+1, CV_8UC3, cv::Scalar(0));
//            for(auto f: templ[0].features){
//                cv::circle(show_templ, {f.x, f.y}, 1, {0, 0, 255}, -1);
//            }
//            for(auto f: templ[1].features){
//                cv::circle(show_templ, {f.x, f.y}, 1, {0, 255, 0}, -1);
//            }
//            cv::imshow("templ", show_templ);
//            cv::waitKey(0);

            int r = 40;
            cout << "\nx: " << match.x << "\ty: " << match.y
                 << "\tsimilarity: "<< match.similarity <<endl;
            cout << "class_id: " << match.class_id << "\ttemplate_id: " << match.template_id <<endl;
            cv::circle(draw, cv::Point(match.x+r,match.y+r), r, cv::Scalar(255, 0 ,255), 2);
            cv::putText(draw, to_string(int(round(match.similarity))),
                        cv::Point(match.x+r-10, match.y-3), FONT_HERSHEY_PLAIN, 1.4, cv::Scalar(0,255,255));
        }
        std::cout << "i: " << i << std::endl;
        imshow("rgb", draw);

        waitKey(0);
//        imwrite(prefix+"scaleTest/" +i_str+ ".png", draw);
    }
    cout << "dataset_test end line" << endl;
}

void view_angle(){
    cv::Mat circle = cv::imread("/home/meiqua/6DPose/linemodLevelup/test/circle.png");
    cv::cvtColor(circle, circle, CV_BGR2GRAY);

    cv::Mat sx, sy, angle;
    cv::Sobel(circle, sx, CV_32F, 1, 0, 3);
    cv::Sobel(circle, sy, CV_32F, 0, 1, 3);
    cv::phase(sx, sy, angle, true);
    normalize(angle, angle, 0, 255, NORM_MINMAX);
    angle.convertTo(angle,CV_8UC1);
    cv::imshow("angle", angle);
    cv::waitKey(0);
}

void renderer_test(){
    int width = 640; int height = 480;
    string dataset_prefix = "/home/meiqua/patch_linemod/public/datasets/doumanoglou/";
    string model_path = dataset_prefix + "models/obj_01.ply";

    Mat K = (Mat_<float>(3,3) << 572.4114, 0.0, 325.2611, 0.0, 573.57043, 242.04899, 0.0, 0.0, 1.0);

    PoseRenderer renderer(model_path);
    renderer.set_K_width_height(K, width, height);

    float data[] = {-1.11833259e-01, -9.77041960e-01, 1.81334868e-01, 0.,
                    8.67027938e-01, -6.77462015e-03, 4.98213470e-01, 0.,
                    -4.85546976e-01, 2.12939233e-01, 8.47880304e-01, 545., 0., 0.,
                    0., 1. };
    Mat pose = Mat(4, 4, CV_32F, data);
    vector<Mat> init_poses;
    init_poses.push_back(pose);
    auto deps = renderer.render_depth(init_poses);

//    imshow("dep", deps[0] > 0);
//    waitKey(1000);
}

void view_structure(){
    auto templ_structure = hinter_sampling(4, 1, 0, 2*CV_PI, -0.5*CV_PI, 0.5*CV_PI, -CV_PI, CV_PI, CV_PI*10);
    cout << "end" << endl;
}

void is_similar_test(){
    string model_path = "/home/meiqua/patch_linemod/public/datasets/hinterstoisser/models/obj_06.ply";
    PoseRenderer renderer(model_path);

    Mat K = (Mat_<float>(3,3) << 572.4114, 0.0, 325.2611, 0.0, 573.57043, 242.04899, 0.0, 0.0, 1.0);
    renderer.set_K_width_height(K, 640, 480);

    Mat R_ren = (Mat_<float>(3,3) << 0.34768538, 0.93761126, 0.00000000, 0.70540612,
                 -0.26157897, -0.65877056, -0.61767070, 0.22904489, -0.75234390);
    Mat t_ren = (Mat_<float>(3,1) << 0.0, 0.0, 300.0);
    Mat pose1 = Mat::eye(4, 4, CV_32F);
    R_ren.copyTo(pose1(Rect(0, 0, 3, 3)));
    t_ren.copyTo(pose1(Rect(3, 0, 1, 3)));

    Mat pose2 = pose1.clone();
//    pose2.at<float>(0, 3) += 20;
//    pose2.at<float>(1, 3) += 20;

    Mat delta_R = eulerAnglesToRotationMatrix({15.0f/180*3.14f, 15.0f/180*3.14f, 0});
    Mat R_ren2 = delta_R * R_ren;
    R_ren2.copyTo(pose2(Rect(0, 0, 3, 3)));

    vector<Mat> pose_v(2); pose_v[0] = pose1; pose_v[1] = pose2;
    cout << "pose1: " << endl;
    cout << pose1 << endl;
    cout << "pose2: " << endl;
    cout <<pose2<<endl;

    auto masks = renderer.render_mask(pose_v);
    Mat diff;
    bitwise_xor(masks[0], masks[1], diff);
    imshow("mask0", masks[0]);
    imshow("mask1", masks[1]);
    imshow("diff", diff);
    waitKey(0);
    linemodLevelup::Detector detector;
    detector.is_similar(pose1, pose2, 2, 8, renderer);
}

int main(){
    is_similar_test();
    return 0;
}
