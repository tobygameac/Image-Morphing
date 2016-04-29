#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <omp.h>
#include <opencv\cv.hpp>

#include "warping.h"

namespace ImageMorphing {

  inline std::pair<cv::Point2d, cv::Point2d> LineInterpolation(
    const std::pair<cv::Point2d, cv::Point2d> &l1, const std::pair<cv::Point2d, cv::Point2d> &l2,
    double t) {
    //std::pair<cv::Point2d, cv::Point2d> result_line(l1.first * (1 - t) + l2.first * t, l1.second * (1 - t) + l2.second * t);
    //return result_line;

    double new_line_length = LineLength(l1) * (1 - t) + LineLength(l2) * t;
    cv::Point2d new_line_midpoint = LineMidpoint(l1) * (1 - t) + LineMidpoint(l2) * t;
    double new_line_orientation = LineOrientation(l1) * (1 - t) + LineOrientation(l2) * t;

    cv::Point2d new_line_direction = new_line_length * 0.5 * cv::Point2d(std::cos(new_line_orientation), std::sin(new_line_orientation));

    std::pair<cv::Point2d, cv::Point2d> result_line(new_line_midpoint - new_line_direction, new_line_midpoint + new_line_direction);

    return result_line;
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

    //cv::Mat warpped_source_image = ImageWarping(source_image, source_feature_lines, feature_lines_at_t, a, b, p);
    cv::Mat warpped_source_image = ImageWarpingWithMeshOptimization(source_image, source_feature_lines, feature_lines_at_t, a, b, p, 20);
    //cv::Mat warpped_destination_image = ImageWarping(destination_image, destination_feature_lines, feature_lines_at_t, a, b, p);
    cv::Mat warpped_destination_image = ImageWarpingWithMeshOptimization(destination_image, destination_feature_lines, feature_lines_at_t, a, b, p, 20);

    cv::Mat result_image(source_image.size(), source_image.type());

    //return warpped_source_image;

    //return warpped_destination_image;

#pragma omp parallel for
    for (int r = 0; r < result_image.rows; ++r) {
#pragma omp parallel for
      for (int c = 0; c < result_image.cols; ++c) {
        result_image.at<cv::Vec3b>(r, c) = (cv::Vec3d)warpped_source_image.at<cv::Vec3b>(r, c) * (1 - t) + (cv::Vec3d)warpped_destination_image.at<cv::Vec3b>(r, c) * (t);
      }
    }

    return result_image;
  }
}