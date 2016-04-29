#pragma once

#define IL_STD

#include <memory>
#include <vector>

#include <GL\glew.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <ilcplex\ilocplex.h>
#include <omp.h>
#include <opencv\cv.hpp>

#include "application_form.h"
#include "gl_mesh.h"
#include "gl_texture.h"
#include "graph.h"

namespace ImageMorphing {

  const float FOVY = 45.0f;

  const bool DRAW_MESH = false;

  GLuint CreateFrameBuffer(const size_t width, const size_t height) {
    GLint old_frame_buffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_frame_buffer);

    GLuint frame_buffer_id = 0;
    glGenFramebuffers(1, &frame_buffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id);

    GLuint rendered_texture;
    glGenTextures(1, &rendered_texture);

    glBindTexture(GL_TEXTURE_2D, rendered_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_texture, 0);

    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, old_frame_buffer);

    return frame_buffer_id;
  }

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

  cv::Mat ImageWarping(const cv::Mat &source_image,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &source_feature_lines,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &destination_feature_lines,
    const double a, const double b, const double p) {

    cv::Mat warpped_image = source_image.clone();

#pragma omp parallel for
    for (int r = 0; r < warpped_image.rows; ++r) {
#pragma omp parallel for
      for (int c = 0; c < warpped_image.cols; ++c) {

        cv::Vec3d total_warpped_color(0, 0, 0);

        std::vector<double> lines_weight(destination_feature_lines.size());
        double weight_sum = 0;

        for (size_t i = 0; i < destination_feature_lines.size(); ++i) {
          cv::Point2d p_x = (cv::Point2d(c, r) - destination_feature_lines[i].first);
          cv::Point2d p_q = destination_feature_lines[i].second - destination_feature_lines[i].first;

          double u = p_x.ddot(p_q) / SqrLineLength(destination_feature_lines[i]);
          double v = p_x.ddot(Perpendicular(p_q)) / LineLength(destination_feature_lines[i]);

          cv::Point2d p_q_prime = source_feature_lines[i].second - source_feature_lines[i].first;

          cv::Point2d warpped_position = source_feature_lines[i].first + u * p_q_prime + v * Perpendicular(p_q_prime) / LineLength(source_feature_lines[i]);

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

          warpped_position.x = std::max(0.0, warpped_position.x);
          warpped_position.x = std::min(source_image.cols - 1.0, warpped_position.x);

          warpped_position.y = std::max(0.0, warpped_position.y);
          warpped_position.y = std::min(source_image.rows - 1.0, warpped_position.y);

          warpped_color = BilinearInterpolationPixelValue(source_image, warpped_position);

          total_warpped_color += warpped_color * lines_weight[i];
        }

        total_warpped_color /= weight_sum;

        warpped_image.at<cv::Vec3b>(r, c) = total_warpped_color;
      }
    }

    return warpped_image;
  }

  void BuildGridMeshAndGraphForImage(const cv::Mat &image, GLMesh &target_mesh, Graph<glm::vec2> &target_graph, float grid_size) {
    target_graph = Graph<glm::vec2>();

    size_t mesh_column_count = (size_t)(image.size().width / grid_size) + 1;
    size_t mesh_row_count = (size_t)(image.size().height / grid_size) + 1;

    float real_mesh_width = image.size().width / (float)(mesh_column_count - 1);
    float real_mesh_height = image.size().height / (float)(mesh_row_count - 1);

    for (size_t r = 0; r < mesh_row_count; ++r) {
      for (size_t c = 0; c < mesh_column_count; ++c) {
        target_graph.vertices_.push_back(glm::vec2(c * real_mesh_width, r * real_mesh_height));
      }
    }

    target_mesh = GLMesh();
    target_mesh.vertices_type = GL_QUADS;

    for (size_t r = 0; r < mesh_row_count - 1; ++r) {
      for (size_t c = 0; c < mesh_column_count - 1; ++c) {
        std::vector<size_t> vertex_indices;

        size_t base_index = r * mesh_column_count + c;
        vertex_indices.push_back(base_index);
        vertex_indices.push_back(base_index + mesh_column_count);
        vertex_indices.push_back(base_index + mesh_column_count + 1);
        vertex_indices.push_back(base_index + 1);

        if (!c) {
          target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[0], vertex_indices[1])));
        }

        target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[1], vertex_indices[2])));
        target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[3], vertex_indices[2])));

        if (!r) {
          target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[0], vertex_indices[3])));
        }

        for (const size_t vertex_index : vertex_indices) {
          target_mesh.vertices_.push_back(glm::vec3(target_graph.vertices_[vertex_index].x, target_graph.vertices_[vertex_index].y, 0.0f));
          target_mesh.uvs_.push_back(glm::vec2(target_graph.vertices_[vertex_index].x / (float)image.size().width, target_graph.vertices_[vertex_index].y / (float)image.size().height));
        }
      }
    }
  }

  cv::Mat ImageWarpingWithMeshOptimization(const cv::Mat &source_image,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &original_source_feature_lines,
    const std::vector<std::pair<cv::Point2d, cv::Point2d> > &original_destination_feature_lines,
    const double a, const double b, const double p,
    const size_t grid_size) {

    std::vector<std::pair<cv::Point2d, cv::Point2d> > source_feature_lines = original_source_feature_lines;
    std::vector<std::pair<cv::Point2d, cv::Point2d> > destination_feature_lines = original_destination_feature_lines;

    for (auto &line : source_feature_lines) {
      line.first.y = source_image.rows - line.first.y;
      line.second.y = source_image.rows - line.second.y;
    }

    for (auto &line : destination_feature_lines) {
      line.first.y = source_image.rows - line.first.y;
      line.second.y = source_image.rows - line.second.y;
    }

    Graph<glm::vec2> image_graph;
    GLMesh grid_mesh;
    BuildGridMeshAndGraphForImage(source_image, grid_mesh, image_graph, grid_size);

    IloEnv env;

    IloNumVarArray x(env);
    IloExpr expr(env);
    IloRangeArray hard_constraint(env);

    for (size_t vertex_index = 0; vertex_index < image_graph.vertices_.size(); ++vertex_index) {
      x.add(IloNumVar(env, image_graph.vertices_[0].x, image_graph.vertices_[0].x + source_image.cols));
      x.add(IloNumVar(env, image_graph.vertices_[0].y, image_graph.vertices_[0].y + source_image.rows));
    }

    size_t mesh_column_count = (size_t)(source_image.cols / grid_size) + 1;
    size_t mesh_row_count = (size_t)(source_image.rows / grid_size) + 1;

    // Boundary constraint
    for (size_t row = 0; row < mesh_row_count; ++row) {
      size_t vertex_index = row * mesh_column_count;
      hard_constraint.add(x[vertex_index * 2] == image_graph.vertices_[0].x);

      vertex_index = row * mesh_column_count + mesh_column_count - 1;
      hard_constraint.add(x[vertex_index * 2] == image_graph.vertices_[0].x + source_image.cols);
    }

    for (size_t column = 0; column < mesh_column_count; ++column) {
      size_t vertex_index = column;
      hard_constraint.add(x[vertex_index * 2 + 1] == image_graph.vertices_[0].y);

      vertex_index = (mesh_row_count - 1) * mesh_column_count + column;
      hard_constraint.add(x[vertex_index * 2 + 1] == image_graph.vertices_[0].y + source_image.rows);
    }

    // Avoid flipping
    for (size_t row = 0; row < mesh_row_count; ++row) {
      for (size_t column = 1; column < mesh_column_count; ++column) {
        size_t vertex_index_right = row * mesh_column_count + column;
        size_t vertex_index_left = row * mesh_column_count + column - 1;
        hard_constraint.add((x[vertex_index_right * 2] - x[vertex_index_left * 2]) >= 1e-4);
      }
    }

    for (size_t row = 1; row < mesh_row_count; ++row) {
      for (size_t column = 0; column < mesh_column_count; ++column) {
        size_t vertex_index_down = row * mesh_column_count + column;
        size_t vertex_index_up = (row - 1) * mesh_column_count + column;
        hard_constraint.add((x[vertex_index_down * 2 + 1] - x[vertex_index_up * 2 + 1]) >= 1e-4);
      }
    }

    const double GRID_WEIGHT = 0.1;
    const double WARPPED_POSITION_WEIGHT = 1;
    const double TRANSFORMATION_WEIGHT = 1;

    for (const Edge &edge : image_graph.edges_) {
      size_t v1_index = edge.edge_indices_pair_.first;
      size_t v2_index = edge.edge_indices_pair_.first;

      glm::vec2 &v1 = image_graph.vertices_[v1_index];
      glm::vec2 &v2 = image_graph.vertices_[v2_index];

      //expr += GRID_WEIGHT * IloPower((x[v1_index * 2] - x[v2_index * 2]) - (v1.x - v2.x), 2);
      //expr += GRID_WEIGHT * IloPower((x[v1_index * 2 + 1] - x[v2_index * 2 + 1]) - (v1.y - v2.y), 2);
    }

    for (size_t j = 0; j < image_graph.vertices_.size(); ++j) {
      glm::vec2 &vertex = image_graph.vertices_[j];
      for (size_t i = 0; i < source_feature_lines.size(); ++i) {
        cv::Point2d p_x = (cv::Point2d(vertex.x, vertex.y) - source_feature_lines[i].first);
        cv::Point2d p_q = source_feature_lines[i].second - source_feature_lines[i].first;

        double u = p_x.ddot(p_q) / SqrLineLength(source_feature_lines[i]);
        double v = p_x.ddot(Perpendicular(p_q)) / LineLength(source_feature_lines[i]);

        cv::Point2d p_q_prime = destination_feature_lines[i].second - destination_feature_lines[i].first;

        cv::Point2d warpped_position = destination_feature_lines[i].first + u * p_q_prime + v * Perpendicular(p_q_prime) / LineLength(destination_feature_lines[i]);

        double distance_with_line = std::abs(v);

        if (u < 0) {
          distance_with_line = LineLength(std::pair<cv::Point2d, cv::Point2d>(cv::Point2d(vertex.x, vertex.y), source_feature_lines[i].first));
        }

        if (u > 1) {
          distance_with_line = LineLength(std::pair<cv::Point2d, cv::Point2d>(cv::Point2d(vertex.x, vertex.y), source_feature_lines[i].second));
        }

        double lines_weight = std::pow(std::pow(LineLength(source_feature_lines[i]), p) / (a + distance_with_line), b);

        warpped_position.x = std::max(0.0, warpped_position.x);
        warpped_position.x = std::min(source_image.cols - 1.0, warpped_position.x);

        warpped_position.y = std::max(0.0, warpped_position.y);
        warpped_position.y = std::min(source_image.rows - 1.0, warpped_position.y);

        expr += WARPPED_POSITION_WEIGHT * lines_weight * IloPower(x[j * 2] - warpped_position.x, 2);
        expr += WARPPED_POSITION_WEIGHT * lines_weight * IloPower(x[j * 2 + 1] - warpped_position.y, 2);
      }
    }

    // Maintain the transformation between edge and feature line
    // This term isn't that good so far
    for (const Edge &edge : image_graph.edges_) {
      break;
      for (size_t i = 0; i < destination_feature_lines.size(); ++i) {

        double c_x = source_feature_lines[i].first.x - source_feature_lines[i].second.x;
        double c_y = source_feature_lines[i].first.y - source_feature_lines[i].second.y;

        double original_matrix_a = c_x;
        double original_matrix_b = c_y;
        double original_matrix_c = c_y;
        double original_matrix_d = -c_x;

        double matrix_rank = original_matrix_a * original_matrix_d - original_matrix_b * original_matrix_c;

        if (fabs(matrix_rank) <= 1e-9) {
          matrix_rank = (matrix_rank > 0 ? 1 : -1) * 1e-9;
        }

        double matrix_a = original_matrix_d / matrix_rank;
        double matrix_b = -original_matrix_b / matrix_rank;
        double matrix_c = -original_matrix_c / matrix_rank;
        double matrix_d = original_matrix_a / matrix_rank;

        const glm::vec2 &v_1 = image_graph.vertices_[edge.edge_indices_pair_.first];
        const glm::vec2 &v_2 = image_graph.vertices_[edge.edge_indices_pair_.second];

        double e_x = v_1.x - v_2.x;
        double e_y = v_1.y - v_2.y;

        double t_s = matrix_a * e_x + matrix_b * e_y;
        double t_r = matrix_c * e_x + matrix_d * e_y;

        glm::vec2 edge_midpoint = (v_1 + v_2) / 2.0f;

        glm::vec2 source_v_1(source_feature_lines[i].first.x, source_feature_lines[i].first.y);
        glm::vec2 source_v_2(source_feature_lines[i].second.x, source_feature_lines[i].second.y);
        glm::vec2 source_midpoint = (source_v_1 + source_v_2) / 2.0f;

        double distance_with_line = std::sqrt(std::pow(source_midpoint.x - edge_midpoint.x, 2.0) + std::pow(source_midpoint.y - edge_midpoint.y, 2.0));
        double lines_weight = std::pow(std::pow(LineLength(destination_feature_lines[i]), p) / (a + distance_with_line), b);

        glm::vec2 destination_v_1(destination_feature_lines[i].first.x, destination_feature_lines[i].first.y);
        glm::vec2 destination_v_2(destination_feature_lines[i].second.x, destination_feature_lines[i].second.y);
        glm::vec2 destination_midpoint = (destination_v_1 + destination_v_2) / 2.0f;

        double d_x = destination_v_1.x - destination_v_2.x;
        double d_y = destination_v_1.y - destination_v_2.y;

        expr += TRANSFORMATION_WEIGHT * lines_weight *
          IloPower((x[edge.edge_indices_pair_.first * 2] - x[edge.edge_indices_pair_.second * 2]) -
            (t_s * d_x + t_r * d_y), 2);

        expr += TRANSFORMATION_WEIGHT * lines_weight *
          IloPower((x[edge.edge_indices_pair_.first * 2 + 1] - x[edge.edge_indices_pair_.second * 2 + 1]) -
            (-t_r * d_x + t_s * d_y), 2);
      }
    }

    IloModel model(env);

    model.add(IloMinimize(env, expr));

    model.add(hard_constraint);

    IloCplex cplex(model);

    cplex.setOut(env.getNullStream());

    if (!cplex.solve()) {
      std::cout << "Failed to optimize the model.\n";
    } else {
    }

    IloNumArray result(env);

    cplex.getValues(result, x);

    for (size_t vertex_index = 0; vertex_index < image_graph.vertices_.size(); ++vertex_index) {
      image_graph.vertices_[vertex_index].x = result[vertex_index * 2];
      image_graph.vertices_[vertex_index].y = result[vertex_index * 2 + 1];
    }

    model.end();
    cplex.end();
    env.end();

    grid_mesh.vertices_.clear();

    for (int r = 0; r < mesh_row_count - 1; ++r) {
      for (int c = 0; c < mesh_column_count - 1; ++c) {
        std::vector<size_t> vertex_indices;

        size_t base_index = r * (mesh_column_count)+c;
        vertex_indices.push_back(base_index);
        vertex_indices.push_back(base_index + mesh_column_count);
        vertex_indices.push_back(base_index + mesh_column_count + 1);
        vertex_indices.push_back(base_index + 1);

        for (const auto &vertex_index : vertex_indices) {
          grid_mesh.vertices_.push_back(glm::vec3(image_graph.vertices_[vertex_index].x, image_graph.vertices_[vertex_index].y, 0));
        }
      }
    }

    GLTexture::SetGLTexture(source_image, &grid_mesh.texture_id_);

    grid_mesh.Upload();

    GLint old_frame_buffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_frame_buffer);

    GLuint frame_buffer_id = CreateFrameBuffer(source_image.cols, source_image.rows);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id);

    glViewport(0, 0, source_image.cols, source_image.rows);

    double cotanget_of_half_of_fovy = 1.0 / tan(glm::radians(FOVY / 2.0f));

    glm::vec3 eye_position = glm::vec3(source_image.cols / 2.0f, source_image.rows / 2.0f, cotanget_of_half_of_fovy * (source_image.rows / 2.0));
    glm::vec3 look_at_position = glm::vec3(source_image.cols / 2.0f, source_image.rows / 2.0f, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect_ratio = source_image.cols / (float)source_image.rows;

    glm::mat4 projection_matrix = glm::perspective(glm::radians(FOVY), aspect_ratio, 0.01f, 10000.0f);

    glm::mat4 view_matrix = glm::lookAt(eye_position, look_at_position, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 modelview_matrix = glm::mat4(1.0f);

    glUniformMatrix4fv(shader_uniform_projection_matrix_id, 1, GL_FALSE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(shader_uniform_view_matrix_id, 1, GL_FALSE, glm::value_ptr(view_matrix));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    grid_mesh.Draw(modelview_matrix);

    if (DRAW_MESH) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      grid_mesh.colors_ = std::vector<glm::vec3>(grid_mesh.vertices_.size(), glm::vec3(1, 0, 0));
      grid_mesh.uvs_.clear();
      grid_mesh.Upload();
      grid_mesh.Draw(modelview_matrix);
    }

    std::vector<unsigned char> screen_image_data(3 * source_image.cols * source_image.rows);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, source_image.cols, source_image.rows, GL_BGR, GL_UNSIGNED_BYTE, &screen_image_data[0]);

    cv::Mat buffer_image(source_image.rows, source_image.cols, CV_8UC3, &screen_image_data[0]);
    cv::flip(buffer_image, buffer_image, 0);

    cv::Mat warpped_image = buffer_image.clone();

    glBindFramebuffer(GL_FRAMEBUFFER, old_frame_buffer);

    return warpped_image;
  }
}