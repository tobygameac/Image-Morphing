#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <opencv\cv.hpp>

namespace ImageMorphing {

  inline double SqrLineLength(const std::pair<cv::Point2f, cv::Point2f> &line) {
    return std::pow(line.first.x - line.second.x, 2.0) + std::pow(line.first.y - line.second.y, 2.0);
  }

  inline double LineLength(const std::pair<cv::Point2f, cv::Point2f> &line) {
    return std::sqrt(SqrLineLength(line));
  }

  inline double Dot(const cv::Point2f &v1, const cv::Point2f &v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
  }

  inline cv::Point2f Perpendicular(const cv::Point2f &v) {
    return cv::Point2f(-v.y, v.x);
  }

  inline cv::Vec3f BilinearInterpolationPixelValue(const cv::Mat &source_image, const cv::Point2f &pixel_position) {
    cv::Vec3f upper_left = source_image.at<cv::Vec3b>(pixel_position.y, pixel_position.x);
    if (pixel_position.x >= source_image.cols - 1 || pixel_position.y >= source_image.rows - 1) {
      return upper_left;
    }
    cv::Vec3f lower_left = source_image.at<cv::Vec3b>(pixel_position.y + 1, pixel_position.x);
    cv::Vec3f upper_right = source_image.at<cv::Vec3b>(pixel_position.y, pixel_position.x + 1);
    cv::Vec3f lower_right = source_image.at<cv::Vec3b>(pixel_position.y + 1, pixel_position.x + 1);

    float t1 = pixel_position.x - (int)pixel_position.x;
    float t2 = pixel_position.y - (int)pixel_position.y;

    cv::Vec3f upper_pixel_value = upper_left * (1 - t1) + upper_right * t1;
    cv::Vec3f lower_pixel_value = lower_left * (1 - t1) + lower_right * t1;

    return upper_pixel_value * (1 - t2) + lower_pixel_value * t2;
  }
  
  cv::Mat ImageWarping(const cv::Mat &source_image,
    std::vector<std::pair<cv::Point2f, cv::Point2f> > &source_feature_lines,
    std::vector<std::pair<cv::Point2f, cv::Point2f> > &destination_feature_lines,
    const float a, const float b, const float p) {

    cv::Mat warpped_image = source_image.clone();

    for (int r = 0; r < warpped_image.rows; ++r) {
      for (int c = 0; c < warpped_image.cols; ++c) {

        cv::Vec3d total_warpped_color(0, 0, 0);

        std::vector<double> lines_weight(destination_feature_lines.size());
        double weight_sum = 0;

        for (size_t i = 0; i < destination_feature_lines.size(); ++i) {
          double u = Dot(cv::Point2f(c, r) - destination_feature_lines[i].first, destination_feature_lines[i].second - destination_feature_lines[i].first) / SqrLineLength(destination_feature_lines[i]);

          double v = Dot(cv::Point2f(c, r) - destination_feature_lines[i].first, Perpendicular(destination_feature_lines[i].second - destination_feature_lines[i].first)) / LineLength(destination_feature_lines[i]);

          cv::Point2f warpped_position = source_feature_lines[i].first + u * (source_feature_lines[i].second - source_feature_lines[i].first) + v * Perpendicular(source_feature_lines[i].second - source_feature_lines[i].first) / LineLength(source_feature_lines[i]);

          double distance_with_line = std::abs(v);

          if (u < 0) {
            distance_with_line = LineLength(std::pair<cv::Point2f, cv::Point2f>(cv::Point2f(c, r), destination_feature_lines[i].first));
          }

          if (u > 1) {
            distance_with_line = LineLength(std::pair<cv::Point2f, cv::Point2f>(cv::Point2f(c, r), destination_feature_lines[i].second));
          }

          lines_weight[i] = std::pow(std::pow(LineLength(destination_feature_lines[i]), p) / (a + distance_with_line), b);
          weight_sum += lines_weight[i];

          cv::Vec3d warpped_color(0, 0, 0);
          if (warpped_position.x >= 0 && warpped_position.x < warpped_image.cols && warpped_position.y >= 0 && warpped_position.y < warpped_image.rows) {
            warpped_color = BilinearInterpolationPixelValue(source_image, warpped_position);
          }
          total_warpped_color += warpped_color * lines_weight[i];
        }

        total_warpped_color /= weight_sum;

        warpped_image.at<cv::Vec3b>(r, c) = total_warpped_color;
      }
    }

    return warpped_image;
  }

  cv::Mat Morphing(const cv::Mat &source_image, const cv::Mat &destination_image, const float t,
    std::vector<std::pair<cv::Point2f, cv::Point2f> > &source_feature_lines,
    std::vector<std::pair<cv::Point2f, cv::Point2f> > &destination_feature_lines,
    const float a, const float b, const float p) {
    if (t < 0 || t > 1) {
      std::cout << "Value of t must be in range[0, 1]\n";
      return source_image;
    }

    if (source_feature_lines.size() != destination_feature_lines.size()) {
      std::cout << "Number of feature lines are not matching\n";
      return source_image;
    }

    std::vector<std::pair<cv::Point2f, cv::Point2f> > feature_lines_at_t(source_feature_lines.size());

    for (size_t i = 0; i < feature_lines_at_t.size(); ++i) {
      feature_lines_at_t[i].first = source_feature_lines[i].first * (1 - t) + destination_feature_lines[i].first * t;
      feature_lines_at_t[i].second = source_feature_lines[i].second * (1 - t) + destination_feature_lines[i].second * t;
    }

    cv::Mat warpped_source_image = ImageWarping(source_image, source_feature_lines, feature_lines_at_t, a, b, p);

    cv::Mat warpped_destination_image = ImageWarping(destination_image, destination_feature_lines, feature_lines_at_t, a, b, p);

    cv::Mat result_image = source_image.clone();

    for (int r = 0; r < result_image.rows; ++r) {
      for (int c = 0; c < result_image.cols; ++c) {
        result_image.at<cv::Vec3b>(r, c) = (cv::Vec3f)warpped_source_image.at<cv::Vec3b>(r, c) * (1 - t) + (cv::Vec3f)warpped_destination_image.at<cv::Vec3b>(r, c) * (t);
      }
    }

    cv::imwrite(std::to_string(t) + "result.jpg", result_image);
    //cv::imshow(std::to_string(t) + "result.jpg", result_image);

    return result_image;
  }
}