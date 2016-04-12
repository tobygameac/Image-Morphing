#include "application_form.h"

namespace ImageMorphing {

  [STAThread]
  int main(array<String^> ^args) {
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    ImageMorphing::ApplicationForm form;
    Application::Run(%form);
    return 0;
  }

  ApplicationForm::ApplicationForm() {
    InitializeComponent();

    picture_boxes = gcnew System::Collections::Generic::List<System::Windows::Forms::PictureBox ^>();

    add_images_tool_strip_menu_item_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);

    start_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    clear_features_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    load_features_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    save_features_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);

    srand(time(0));

    //Test();
  }

  ApplicationForm::~ApplicationForm() {
    if (components) {
      delete components;
    }
  }

  void ApplicationForm::OnButtonsClick(System::Object ^sender, System::EventArgs ^e) {
    if (sender == add_images_tool_strip_menu_item_) {
      OpenFileDialog ^open_images_file_dialog = gcnew OpenFileDialog();
      open_images_file_dialog->Multiselect = true;
      open_images_file_dialog->Filter = "Image files | *.*";
      open_images_file_dialog->Title = "Open an image file.";

      if (open_images_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {

        for each (System::String ^raw_file_path in open_images_file_dialog->FileNames) {

          System::Windows::Forms::PictureBox ^new_picture_box = gcnew System::Windows::Forms::PictureBox();

          new_picture_box->SizeMode = System::Windows::Forms::PictureBoxSizeMode::AutoSize;
          new_picture_box->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &ImageMorphing::ApplicationForm::OnMouseMove);
          new_picture_box->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &ImageMorphing::ApplicationForm::OnMouseDown);

          this->picture_box_panel_->Controls->Add(new_picture_box);

          picture_boxes->Add(new_picture_box);

          source_images.resize(picture_boxes->Count);
          resized_images.resize(picture_boxes->Count);
          images_with_feature_lines.resize(picture_boxes->Count);
          feature_lines_of_images.resize(picture_boxes->Count);
          last_feature_line_of_images.resize(picture_boxes->Count);
          is_drawing_feature_of_images.resize(picture_boxes->Count);

          Bitmap ^source_bitmap = gcnew Bitmap(raw_file_path);
          new_picture_box->Image = source_bitmap;

          std::string file_path = msclr::interop::marshal_as<std::string>(raw_file_path);
          source_images[source_images.size() - 1] = cv::imread(file_path);

          if (picture_boxes->Count >= 2) {
            start_button_->Enabled = true;
          }

          AdjustImagesSize();
        }
      }
    } else if (sender == start_button_) {

      if (picture_boxes->Count < 2) {
        return;
      }

      SaveFileDialog ^save_video_file_dialog = gcnew SaveFileDialog();
      save_video_file_dialog->Filter = "Video file | *.avi";
      save_video_file_dialog->Title = "Save a video file.";

      if (save_video_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        std::string raw_file_path = msclr::interop::marshal_as<std::string>(save_video_file_dialog->FileName);

        std::vector<std::pair<cv::Point2d, cv::Point2d> > &max_amount_features_lines = feature_lines_of_images[0];

        for (const auto &features_lines : feature_lines_of_images) {
          if (features_lines.size() > max_amount_features_lines.size()) {
            max_amount_features_lines = features_lines;
          }
        }

        for (auto &features_lines : feature_lines_of_images) {
          for (size_t i = features_lines.size(); i < max_amount_features_lines.size(); ++i) {
            features_lines.push_back(max_amount_features_lines[i]);
          }
        }

        const double FPS = 30;

        const size_t FRAME_COUNT = System::Decimal::ToInt32(morphing_steps_numeric_up_down_->Value);

        const size_t INTERPOLATION_FRAME_COUNT = 5;
        const double INTERPOLATION_GAP = 1.0 / (double)INTERPOLATION_FRAME_COUNT;

        std::vector<cv::Mat> result_at_t(FRAME_COUNT + 1);

        cv::VideoWriter result_video_writer;

        result_video_writer.open(raw_file_path, 0, FPS, resized_images[0].size());

        for (size_t image_index = 1; image_index < source_images.size(); ++image_index) {

          double t_gap = 1.0 / (double)FRAME_COUNT;

#pragma omp parallel for
          for (int i = 0; i < result_at_t.size(); ++i) {
            double t = t_gap * i;
            result_at_t[i] = Morphing(resized_images[image_index - 1], resized_images[image_index], t, feature_lines_of_images[image_index - 1], feature_lines_of_images[image_index], 1, 2, 0).clone();

            std::cout << "Done : " << image_index << " - " << t << "\n";
          }

          for (const cv::Mat &frame : result_at_t) {
            result_video_writer.write(frame);
          }

          //for (size_t i = 0; i < result_at_t.size(); ++i) {
          //  if (!i) {
          //    if (image_index == 1) {
          //      result_video_writer.write(result_at_t[i]);
          //    }
          //  } else {
          //    for (double delta_t = INTERPOLATION_GAP; delta_t <= 1; delta_t += INTERPOLATION_GAP) {
          //      cv::Mat interpolated_image;
          //      cv::addWeighted(result_at_t[i - 1], (1 - delta_t), result_at_t[i], delta_t, 0.0, interpolated_image);
          //      result_video_writer.write(interpolated_image);
          //    }
          //  }
          //}
        }

        //for (double t = 0; t <= 1; t += t_gap) {
        //  cv::Mat result_at_t = Morphing(resized_source_image, resized_destination_image, t, source_feature_lines, destination_feature_lines, 1, 2, 0);

        //  result_video_writer.write(result_at_t);
        //}

        std::cout << "Done.\n";
      }

    } else if (sender == clear_features_button_) {
      feature_lines_of_images.clear();

      for (size_t &is_drawing_feature : is_drawing_feature_of_images) {
        is_drawing_feature = false;
      }

      PaintPictureBoxWithFeatures();
    } else if (sender == load_features_button_) {
      OpenFileDialog ^open_features_file_dialog = gcnew OpenFileDialog();
      open_features_file_dialog->Filter = "Features file | *.*";
      open_features_file_dialog->Title = "Open a features file.";

      if (open_features_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        std::string raw_file_path = msclr::interop::marshal_as<std::string>(open_features_file_dialog->FileName);
        LoadFeatures(raw_file_path);
      }
    } else if (sender == save_features_button_) {
      SaveFileDialog ^save_features_file_dialog = gcnew SaveFileDialog();
      save_features_file_dialog->Filter = "Features file | *.*";
      save_features_file_dialog->Title = "Save a features file.";

      if (save_features_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        std::string raw_file_path = msclr::interop::marshal_as<std::string>(save_features_file_dialog->FileName);
        SaveFeatures(raw_file_path);
      }
    }
  }

  void ApplicationForm::OnMouseMove(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    for (size_t i = 0; i < picture_boxes->Count; ++i) {
      if (sender == picture_boxes[i]) {
        if (is_drawing_feature_of_images[i]) {
          last_feature_line_of_images[i].second = cv::Point2d(e->X, e->Y);
          PaintPictureBoxWithFeatures();
        }
        break;
      }
    }
  }

  void ApplicationForm::OnMouseDown(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    if (e->Button == System::Windows::Forms::MouseButtons::Left) {
      // New features
      for (size_t i = 0; i < picture_boxes->Count; ++i) {
        if (sender == picture_boxes[i]) {
          if (is_drawing_feature_of_images[i]) {
            last_feature_line_of_images[i].second = cv::Point2d(e->X, e->Y);
            feature_lines_of_images[i].push_back(last_feature_line_of_images[i]);
            PaintPictureBoxWithFeatures();
          } else {
            last_feature_line_of_images[i].first = cv::Point2d(e->X, e->Y);
          }
          is_drawing_feature_of_images[i] = !is_drawing_feature_of_images[i];
          break;
        }
      }
    } else if (e->Button == System::Windows::Forms::MouseButtons::Right) {
      // Remove the last feature
      for (size_t i = 0; i < picture_boxes->Count; ++i) {
        if (sender == picture_boxes[i]) {
          if (!is_drawing_feature_of_images[i]) {
            if (feature_lines_of_images[i].size()) {
              feature_lines_of_images[i].pop_back();
            }
          }
          is_drawing_feature_of_images[i] = false;
          PaintPictureBoxWithFeatures();
          break;
        }
      }
    }
  }

  void ApplicationForm::AdjustImagesSize() {
    if (!picture_boxes->Count) {
      return;
    }

    cv::Size min_size = source_images[0].size();

    for (const cv::Mat &image : source_images) {
      min_size.width = std::min(min_size.width, image.size().width);
      min_size.height = std::min(min_size.height, image.size().height);
    }

    if (resized_images.size() < source_images.size()) {
      resized_images.resize(source_images.size());
    }

    for (size_t i = 0; i < source_images.size(); ++i) {
      resized_images[i] = source_images[i](cv::Rect(0, 0, min_size.width, min_size.height)).clone();
    }

    for (size_t i = 0; i < picture_boxes->Count; ++i) {
      if (!i) {
        picture_boxes[i]->Location = System::Drawing::Point(picture_box_panel_->Location.X + PICTURE_BOX_LOCATION_GAP, picture_box_panel_->Location.Y + PICTURE_BOX_LOCATION_GAP);
      } else {
        picture_boxes[i]->Location = System::Drawing::Point(picture_boxes[i - 1]->Location.X + min_size.width + PICTURE_BOX_LOCATION_GAP, picture_boxes[i - 1]->Location.Y);
      }
    }

    PaintPictureBoxWithFeatures();
  }

  void ApplicationForm::PaintPictureBoxWithFeatures() {

    if (!resized_images.size()) {
      return;
    }

    if (images_with_feature_lines.size() != resized_images.size()) {
      images_with_feature_lines.resize(resized_images.size());
    }

    size_t max_feature_lines_count = 0;

    for (size_t i = 0; i < images_with_feature_lines.size(); ++i) {
      images_with_feature_lines[i] = resized_images[i].clone();
      max_feature_lines_count = std::max(max_feature_lines_count, feature_lines_of_images[i].size());
    }

    for (size_t i = feature_colors.size(); i < max_feature_lines_count + 1; ++i) {
      feature_colors.push_back(cv::Vec3b(rand() % 256, rand() % 256, rand() % 256));
    }

    for (size_t i = 0; i < images_with_feature_lines.size(); ++i) {
      if (is_drawing_feature_of_images[i]) {
        cv::line(images_with_feature_lines[i], last_feature_line_of_images[i].first, last_feature_line_of_images[i].second, feature_colors[feature_lines_of_images[i].size()], FEATURE_LINE_THICKNESS);
      }
    }

    for (size_t i = 0; i < images_with_feature_lines.size(); ++i) {
      for (size_t j = 0; j < feature_lines_of_images[i].size(); ++j) {
        const auto &feature_line = feature_lines_of_images[i][j];
        cv::line(images_with_feature_lines[i], feature_line.first, feature_line.second, feature_colors[j], FEATURE_LINE_THICKNESS);
      }
      picture_boxes[i]->Image = CVMatToBitmap(images_with_feature_lines[i]);
    }
  }

  System::Drawing::Bitmap ^ApplicationForm::CVMatToBitmap(const cv::Mat &mat) {
    //return gcnew System::Drawing::Bitmap(mat.cols, mat.rows, mat.step, System::Drawing::Imaging::PixelFormat::Format24bppRgb, (System::IntPtr)mat.bitmap_data);

    if (mat.type() != CV_8UC3) {
      throw gcnew NotSupportedException("Only images of type CV_8UC3 are supported for conversion to Bitmap");
    }

    // Create the bitmap and get the pointer to the bitmap
    System::Drawing::Imaging::PixelFormat pixel_format(System::Drawing::Imaging::PixelFormat::Format24bppRgb);

    System::Drawing::Bitmap ^bitmap = gcnew System::Drawing::Bitmap(mat.cols, mat.rows, pixel_format);

    System::Drawing::Imaging::BitmapData ^bitmap_data = bitmap->LockBits(System::Drawing::Rectangle(0, 0, mat.cols, mat.rows), System::Drawing::Imaging::ImageLockMode::WriteOnly, pixel_format);

    unsigned char *source_pointer = mat.data;

    unsigned char *destination_pointer = reinterpret_cast<unsigned char*>(bitmap_data->Scan0.ToPointer());

    for (int r = 0; r < bitmap_data->Height; ++r) {
      memcpy(reinterpret_cast<void *>(&destination_pointer[r * bitmap_data->Stride]), reinterpret_cast<void *>(&source_pointer[r * mat.step]), mat.cols * mat.channels());
    }

    bitmap->UnlockBits(bitmap_data);

    return bitmap;
  }

  void ApplicationForm::LoadFeatures(const std::string &file_path) {
    if (feature_lines_of_images.size() < picture_boxes->Count) {
      feature_lines_of_images.resize(picture_boxes->Count);
    }

    for (auto &feature_lines : feature_lines_of_images) {
      feature_lines.clear();
    }

    std::ifstream features_input_stream(file_path);

    std::pair<cv::Point2d, cv::Point2d> feature_line;

    size_t target_index = 0;

    while (features_input_stream >> feature_line.first.x >> feature_line.first.y >> feature_line.second.x >> feature_line.second.y) {
      if (feature_line.first.x < 0) {
        ++target_index;
        continue;
      }

      if (target_index >= feature_lines_of_images.size()) {
        break;
      }

      feature_lines_of_images[target_index].push_back(feature_line);
    }

    PaintPictureBoxWithFeatures();
  }

  void ApplicationForm::SaveFeatures(const std::string &file_path) {
    std::ofstream features_output_stream(file_path);

    for (auto feature_lines : feature_lines_of_images) {
      for (auto feature_line : feature_lines) {
        features_output_stream << feature_line.first.x << " " << feature_line.first.y << " " << feature_line.second.x << " " << feature_line.second.y << "\n";
      }
      features_output_stream << "-1 -1 -1 -1\n";
    }
  }

  void ApplicationForm::Test() {
    //original_source_image = cv::imread("..//data//image038.jpg");
    //original_destination_image = cv::imread("..//data//image036.jpg");
    //AdjustImagesSize();
    //LoadFeatures("..//data//features.txt");
  }
}