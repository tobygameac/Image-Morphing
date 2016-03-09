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

        AdjustImagesSize();
      }
    }
  }

  void ApplicationForm::AdjustImagesSize() {
    if (original_source_image.empty() || original_destination_image.empty()) {
      return;
    }

    const size_t ORIGINAL_SOURCE_IMAGE_AREA = (size_t)original_source_image.rows * (size_t)original_source_image.cols;
    const size_t ORIGINAL_DESTINATION_IMAGE_AREA = (size_t)original_destination_image.rows * (size_t)original_destination_image.cols;
    const cv::Size RESIZED_IMAGE_SIZE = (ORIGINAL_SOURCE_IMAGE_AREA >= ORIGINAL_DESTINATION_IMAGE_AREA) ? original_source_image.size() : original_destination_image.size();
    cv::resize(original_source_image, resized_source_image, RESIZED_IMAGE_SIZE);
    cv::resize(original_destination_image, resized_destination_image, RESIZED_IMAGE_SIZE);

    source_picture_box_->Image = CVMatToBitmap(resized_source_image);
    destination_picture_box_->Image = CVMatToBitmap(resized_destination_image);

    destination_picture_box_->Location = System::Drawing::Point(source_picture_box_->Location.X + resized_source_image.cols + PICTURE_BOX_LOCATION_GAP, source_picture_box_->Location.Y);
  }

  System::Drawing::Bitmap ^ApplicationForm::CVMatToBitmap(const cv::Mat &mat) {
    return gcnew System::Drawing::Bitmap(mat.cols, mat.rows, mat.step, System::Drawing::Imaging::PixelFormat::Format24bppRgb, (System::IntPtr)mat.data);
  }

  void ApplicationForm::Test() {
    original_source_image = cv::imread("..//data//butterfly.jpg");
    original_destination_image = cv::imread("..//data//butterfly2.jpg");
    AdjustImagesSize();
  }
}