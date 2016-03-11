#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <opencv\cv.hpp>

namespace ImageMorphing {

  inline double SqrLineLength(const std::pair<cv::Point2d, cv::Point2d> &line) {
    return std::pow(line.second.x - line.first.x, 2.0) + std::pow(line.second.y - line.first.y, 2.0);
  }

  inline double LineLength(const std::pair<cv::Point2d, cv::Point2d> &line) {
    return std::sqrt(SqrLineLength(line));
  }

  inline cv::Point2d LineMidpoint(const std::pair<cv::Point2d, cv::Point2d> &line) {
    return cv::Point2d((line.first.x + line.second.x) * 0.5, (line.first.y + line.second.y) * 0.5);
  }

  inline double LineOrientation(const std::pair<cv::Point2d, cv::Point2d> &line) {
    return std::atan2(line.second.y - line.first.y, line.second.x - line.first.x);
  }

  inline cv::Point2d Perpendicular(const cv::Point2d &v) {
    return cv::Point2d(-v.y, v.x);
  }

  inline cv::Vec3d BilinearInterpolationPixelValue(const cv::Mat &source_image, const cv::Point2d &pixel_position) {
    cv::Vec3d upper_left = source_image.at<cv::Vec3b>(std::floor(pixel_position.y), std::floor(pixel_position.x));
    if (pixel_position.x >= source_image.cols - 1 || pixel_position.y >= source_image.rows - 1) {
      return upper_left;
    }
    cv::Vec3d lower_left = source_image.at<cv::Vec3b>(std::ceil(pixel_position.y), std::floor(pixel_position.x));
    cv::Vec3d upper_right = source_image.at<cv::Vec3b>(std::floor(pixel_position.y), std::ceil(pixel_position.x));
    cv::Vec3d lower_right = source_image.at<cv::Vec3b>(std::ceil(pixel_position.y), std::ceil(pixel_position.x));

    double t1 = pixel_position.x - std::floor(pixel_position.x);
    double t2 = pixel_position.y - std::floor(pixel_position.y);

    cv::Vec3d upper_pixel_value = upper_left * (1 - t1) + upper_right * t1;
    cv::Vec3d lower_pixel_value = lower_left * (1 - t1) + lower_right * t1;

    return upper_pixel_value * (1 - t2) + lower_pixel_value * t2;
  }

  inline std::pair<cv::Point2d, cv::Point2d> LineInterpolation(
    const std::pair<cv::Point2d, cv::Point2d> &l1, const std::pair<cv::Point2d, cv::Point2d> &l2, 
    double t) {
    //std::pair<cv::Point2d, cv::Point2d> result_line(l1.first * (1 - t) + l2.first * t, l1.second * (1 - t) + l2.second * t);

    double new_line_length = LineLength(l1) * (1 - t) + LineLength(l2) * t;
    cv::Point2d new_line_midpoint = LineMidpoint(l1) * (1 - t) + LineMidpoint(l2) * t;
    double new_line_orientation = LineOrientation(l1) * (1 - t) + LineOrientation(l2) * t;

    cv::Point2d new_line_offset = new_line_length * 0.5 * cv::Point2d(std::cos(new_line_orientation), std::sin(new_line_orientation));

    std::pair<cv::Point2d, cv::Point2d> result_line(new_line_midpoint - new_line_offset, new_line_midpoint + new_line_offset);

    return result_line;
  }
  
  cv::Mat ImageWarping(const cv::Mat &source_image,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &source_feature_lines,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &destination_feature_lines,
    const double a, const double b, const double p) {

    cv::Mat warpped_image = source_image.clone();

    for (int r = 0; r < warpped_image.rows; ++r) {
      for (int c = 0; c < warpped_image.cols; ++c) {

        cv::Vec3d total_warpped_color(0, 0, 0);

        std::vector<double> lines_weight(destination_feature_lines.size());
        double weight_sum = 0;

        for (size_t i = 0; i < destination_feature_lines.size(); ++i) {
          cv::Point2d px = (cv::Point2d(c, r) - destination_feature_lines[i].first);
          cv::Point2d pq = destination_feature_lines[i].second - destination_feature_lines[i].first;

          double u = px.ddot(pq) / SqrLineLength(destination_feature_lines[i]);
          double v = px.ddot(Perpendicular(pq)) / LineLength(destination_feature_lines[i]);

          cv::Point2d pq_prime = source_feature_lines[i].second - source_feature_lines[i].first;

          cv::Point2d warpped_position = source_feature_lines[i].first + u * pq_prime + v * Perpendicular(pq_prime) / LineLength(source_feature_lines[i]);

          double distance_with_line = std::abs(v);

          if (u < 0) {
            distance_with_line = LineLength(std::pair<cv::Point2d, cv::Point2d>(cv::Point2d(c, r), destination_feature_lines[i].first));
          }

          if (u > 1) {
            distance_with_line = LineLength(std::pair<cv::Point2d, cv::Point2d>(cv::Point2d(c, r), destination_feature_lines[i].second));
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

  cv::Mat Morphing(const cv::Mat &source_image, const cv::Mat &destination_image, const double t,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &source_feature_lines,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &destination_feature_lines,
    const double a, const double b, const double p) {
    if (t < 0 || t > 1) {
      std::cout << "Value of t must be in range[0, 1]\n";
      return source_image;
    }

    if (source_feature_lines.size() != destination_feature_lines.size()) {
      std::cout << "Number of feature lines are not matching\n";
      return source_image;
    }

    if (!source_feature_lines.size()) {
      std::cout << "No feature line\n";
      return source_image;
    }

    std::vector<std::pair<cv::Point2d, cv::Point2d> > feature_lines_at_t(source_feature_lines.size());

    for (size_t i = 0; i < feature_lines_at_t.size(); ++i) {
      feature_lines_at_t[i] = LineInterpolation(source_feature_lines[i], destination_feature_lines[i], t);
    }

    cv::Mat warpped_source_image = ImageWarping(source_image, source_feature_lines, feature_lines_at_t, a, b, p);

    cv::Mat warpped_destination_image = ImageWarping(destination_image, destination_feature_lines, feature_lines_at_t, a, b, p);

    cv::Mat result_image(source_image.size(), source_image.type());

    for (int r = 0; r < result_image.rows; ++r) {
      for (int c = 0; c < result_image.cols; ++c) {
        result_image.at<cv::Vec3b>(r, c) = (cv::Vec3d)warpped_source_image.at<cv::Vec3b>(r, c) * (1 - t) + (cv::Vec3d)warpped_destination_image.at<cv::Vec3b>(r, c) * (t);
      }
    }

    cv::imwrite(std::to_string(t) + "result.jpg", result_image);

    return result_image;
  }
}