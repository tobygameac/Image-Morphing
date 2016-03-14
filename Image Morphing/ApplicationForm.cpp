#include "ApplicationForm.h"

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

    open_source_image_tool_strip_menu_item_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    open_destination_image_tool_strip_menu_item_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);

    start_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    clear_features_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    load_features_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);
    save_features_button_->Click += gcnew System::EventHandler(this, &ImageMorphing::ApplicationForm::OnButtonsClick);

    source_picture_box_->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &ImageMorphing::ApplicationForm::OnMouseMove);
    destination_picture_box_->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &ImageMorphing::ApplicationForm::OnMouseMove);

    source_picture_box_->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &ImageMorphing::ApplicationForm::OnMouseDown);
    destination_picture_box_->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &ImageMorphing::ApplicationForm::OnMouseDown);

    srand(time(0));

    Test();
  }

  ApplicationForm::~ApplicationForm() {
    if (components) {
      delete components;
    }
  }

  void ApplicationForm::OnButtonsClick(System::Object ^sender, System::EventArgs ^e) {
    if (sender == open_source_image_tool_strip_menu_item_ || sender == open_destination_image_tool_strip_menu_item_) {
      OpenFileDialog ^open_image_file_dialog = gcnew OpenFileDialog();
      open_image_file_dialog->Filter = "Image files | *.*";
      open_image_file_dialog->Title = "Open an image file.";

      if (open_image_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        Bitmap ^source_bitmap = gcnew Bitmap(open_image_file_dialog->FileName);
        (sender == open_source_image_tool_strip_menu_item_ ? source_picture_box_ : destination_picture_box_)->Image = source_bitmap;

        std::string raw_file_path = msclr::interop::marshal_as<std::string>(open_image_file_dialog->FileName);
        (sender == open_source_image_tool_strip_menu_item_ ? original_source_image : original_destination_image) = cv::imread(raw_file_path);

        (sender == open_source_image_tool_strip_menu_item_ ? source_feature_lines : destination_feature_lines).clear();

        AdjustImagesSize();
      }
    } else if (sender == start_button_) {

      for (size_t i = source_feature_lines.size(); i < destination_feature_lines.size(); ++i) {
        source_feature_lines.push_back(destination_feature_lines[i]);
      }

      for (size_t i = destination_feature_lines.size(); i < source_feature_lines.size(); ++i) {
        destination_feature_lines.push_back(source_feature_lines[i]);
      }

      cv::VideoWriter result_video_writer;

      result_video_writer.open("result.avi", 0, 30, resized_source_image.size());


      size_t frame_count = System::Decimal::ToInt32(morphing_steps_numeric_up_down_->Value);

      double t_gap = 1.0 / (double)frame_count;

      std::vector<cv::Mat> result_at_t(frame_count + 1);

#pragma omp parallel for
      for (int i = 0; i < result_at_t.size(); ++i) {
        double t = t_gap * i;
        result_at_t[i] = Morphing(resized_source_image, resized_destination_image, t, source_feature_lines, destination_feature_lines, 1, 2, 0).clone();
        std::cout << "Done : " << t << "\n";
      }

      for (const auto &frame : result_at_t) {
        result_video_writer.write(frame);
      }

      //for (double t = 0; t <= 1; t += t_gap) {
      //  cv::Mat result_at_t = Morphing(resized_source_image, resized_destination_image, t, source_feature_lines, destination_feature_lines, 1, 2, 0);

      //  result_video_writer.write(result_at_t);
      //}

      std::cout << "Done.\n";

    } else if (sender == clear_features_button_) {
      source_feature_lines.clear();
      destination_feature_lines.clear();

      is_drawing_source_features = false;
      is_drawing_destination_features = false;

      PaintPictureBoxWithFeatures();
    } else if (sender == load_features_button_) {
      LoadFeatures();
    } else if (sender == save_features_button_) {
      SaveFeatures();
    }
  }

  void ApplicationForm::OnMouseMove(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    if (sender == source_picture_box_) {
      if (is_drawing_source_features) {
        last_source_feature_line.second = cv::Point2d(e->X, e->Y);
        PaintPictureBoxWithFeatures();
      }
    } else if (sender == destination_picture_box_) {
      if (is_drawing_destination_features) {
        last_destination_feature_line.second = cv::Point2d(e->X, e->Y);
        PaintPictureBoxWithFeatures();
      }
    }
  }

  void ApplicationForm::OnMouseDown(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    if (e->Button == System::Windows::Forms::MouseButtons::Left) {
      // New features
      if (sender == source_picture_box_) {
        if (is_drawing_source_features) {
          last_source_feature_line.second = cv::Point2d(e->X, e->Y);
          source_feature_lines.push_back(last_source_feature_line);
          PaintPictureBoxWithFeatures();
        } else {
          last_source_feature_line.first = cv::Point2d(e->X, e->Y);
        }
        is_drawing_source_features = !is_drawing_source_features;
      } else if (sender == destination_picture_box_) {
        if (is_drawing_destination_features) {
          last_destination_feature_line.second = cv::Point2d(e->X, e->Y);
          destination_feature_lines.push_back(last_destination_feature_line);
          PaintPictureBoxWithFeatures();
        } else {
          last_destination_feature_line.first = cv::Point2d(e->X, e->Y);
        }
        is_drawing_destination_features = !is_drawing_destination_features;
      }
    } else if (e->Button == System::Windows::Forms::MouseButtons::Right) {
      // Remove the last feature
      if (sender == source_picture_box_) {
        if (source_feature_lines.size()) {
          source_feature_lines.pop_back();
          PaintPictureBoxWithFeatures();
        }
        is_drawing_source_features = false;
      } else if (sender == destination_picture_box_) {
        if (destination_feature_lines.size()) {
          destination_feature_lines.pop_back();
          PaintPictureBoxWithFeatures();
        }
        is_drawing_destination_features = false;
      }
    }
  }

  void ApplicationForm::AdjustImagesSize() {
    if (original_source_image.empty() || original_destination_image.empty()) {
      return;
    }

    start_button_->Enabled = true;

    const size_t ORIGINAL_SOURCE_IMAGE_AREA = (size_t)original_source_image.rows * (size_t)original_source_image.cols;
    const size_t ORIGINAL_DESTINATION_IMAGE_AREA = (size_t)original_destination_image.rows * (size_t)original_destination_image.cols;
    const cv::Size RESIZED_IMAGE_SIZE = (ORIGINAL_SOURCE_IMAGE_AREA <= ORIGINAL_DESTINATION_IMAGE_AREA) ? original_source_image.size() : original_destination_image.size();

    cv::resize(original_source_image, resized_source_image, RESIZED_IMAGE_SIZE);
    cv::resize(original_destination_image, resized_destination_image, RESIZED_IMAGE_SIZE);

    destination_picture_box_->Location = System::Drawing::Point(source_picture_box_->Location.X + resized_source_image.cols + PICTURE_BOX_LOCATION_GAP, source_picture_box_->Location.Y);

    PaintPictureBoxWithFeatures();
  }

  void ApplicationForm::PaintPictureBoxWithFeatures() {
    source_features_image = resized_source_image.clone();
    destination_features_image = resized_destination_image.clone();

    const size_t MAX_FEATURE_LINES_COUNT = std::max(source_feature_lines.size(), destination_feature_lines.size()) + 1;

    for (size_t i = feature_colors.size(); i < MAX_FEATURE_LINES_COUNT; ++i) {
      feature_colors.push_back(cv::Vec3b(rand() % 256, rand() % 256, rand() % 256));
    }

    if (is_drawing_source_features) {
      cv::line(source_features_image, last_source_feature_line.first, last_source_feature_line.second, feature_colors[source_feature_lines.size()], FEATURE_LINE_THICKNESS);
    }

    if (is_drawing_destination_features) {
      cv::line(destination_features_image, last_destination_feature_line.first, last_destination_feature_line.second, feature_colors[destination_feature_lines.size()], FEATURE_LINE_THICKNESS);
    }

    for (size_t i = 0; i < source_feature_lines.size(); ++i) {
      cv::line(source_features_image, source_feature_lines[i].first, source_feature_lines[i].second, feature_colors[i], FEATURE_LINE_THICKNESS);
    }

    for (size_t i = 0; i < destination_feature_lines.size(); ++i) {
      cv::line(destination_features_image, destination_feature_lines[i].first, destination_feature_lines[i].second, feature_colors[i], FEATURE_LINE_THICKNESS);
    }

    source_picture_box_->Image = CVMatToBitmap(source_features_image);
    destination_picture_box_->Image = CVMatToBitmap(destination_features_image);
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

  void ApplicationForm::LoadFeatures() {
    source_feature_lines.clear();
    destination_feature_lines.clear();

    std::ifstream features_input_stream("features.txt");

    bool is_source_feature = true;

    std::pair<cv::Point2d, cv::Point2d> feature_line;

    while (features_input_stream >> feature_line.first.x >> feature_line.first.y >> feature_line.second.x >> feature_line.second.y) {
      if (feature_line.first.x < 0) {
        is_source_feature = false;
        continue;
      }
      (is_source_feature ? source_feature_lines : destination_feature_lines).push_back(feature_line);
    }

    PaintPictureBoxWithFeatures();
  }

  void ApplicationForm::SaveFeatures() {
    std::ofstream features_output_stream("features.txt");

    for (const auto &feature : source_feature_lines) {
      features_output_stream << feature.first.x << " " << feature.first.y << " " << feature.second.x << " " << feature.second.y << "\n";
    }

    features_output_stream << "-1 -1 -1 -1\n";

    for (const auto &feature : destination_feature_lines) {
      features_output_stream << feature.first.x << " " << feature.first.y << " " << feature.second.x << " " << feature.second.y << "\n";
    }
  }

  void ApplicationForm::Test() {
    original_source_image = cv::imread("..//data//image038.jpg");
    original_destination_image = cv::imread("..//data//image036.jpg");
    AdjustImagesSize();
    LoadFeatures();
  }
}