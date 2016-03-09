#pragma once

#include <cstdlib>
#include <ctime>

#include <opencv\cv.hpp>

#include <msclr\marshal_cppstd.h>
#using <mscorlib.dll>

namespace ImageMorphing {

  cv::Mat original_source_image;
  cv::Mat original_destination_image;
  cv::Mat resized_source_image;
  cv::Mat resized_destination_image;

  cv::Mat source_features_image;
  cv::Mat destination_features_image;

  std::vector<std::pair<cv::Point2f, cv::Point2f> > source_feature_lines;
  std::vector<std::pair<cv::Point2f, cv::Point2f> > destination_feature_lines;

  std::pair<cv::Point2f, cv::Point2f> last_source_feature_line;
  std::pair<cv::Point2f, cv::Point2f> last_destination_feature_line;

  std::vector<cv::Vec3b> feature_colors;

  bool is_drawing_source_features;
  bool is_drawing_destination_features;

  const size_t FEATURE_LINE_THICKNESS = 3;
  const size_t PICTURE_BOX_LOCATION_GAP = 50;

  using namespace System;
  using namespace System::ComponentModel;
  using namespace System::Collections;
  using namespace System::Windows::Forms;
  using namespace System::Data;
  using namespace System::Drawing;

  /// <summary>
  /// Summary for ApplicationForm
  /// </summary>
  public ref class ApplicationForm : public System::Windows::Forms::Form {

  public:

    ApplicationForm();

  private:

    void OnButtonsClick(System::Object ^sender, System::EventArgs ^e);

    void OnMouseDown(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e);

    void AdjustImagesSize();

    void DrawFeatures();

    System::Drawing::Bitmap ^CVMatToBitmap(const cv::Mat &mat);

    void Test();

  protected:
    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    ~ApplicationForm();

  private:
    /// <summary>
    /// Required designer variable.
    /// </summary>
    System::ComponentModel::Container ^components;
    System::Windows::Forms::Panel ^picture_box_panel_;
    System::Windows::Forms::PictureBox ^destination_picture_box_;
    System::Windows::Forms::PictureBox ^source_picture_box_;
    System::Windows::Forms::MenuStrip ^files_menu_strip_;
    System::Windows::Forms::ToolStripMenuItem ^files_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^open_source_image_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^open_destination_image_tool_strip_menu_item_;

#pragma region Windows Form Designer generated code
    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    void InitializeComponent(void) {
      this->picture_box_panel_ = (gcnew System::Windows::Forms::Panel());
      this->destination_picture_box_ = (gcnew System::Windows::Forms::PictureBox());
      this->source_picture_box_ = (gcnew System::Windows::Forms::PictureBox());
      this->files_menu_strip_ = (gcnew System::Windows::Forms::MenuStrip());
      this->files_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->open_source_image_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->open_destination_image_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->picture_box_panel_->SuspendLayout();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->destination_picture_box_))->BeginInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->source_picture_box_))->BeginInit();
      this->files_menu_strip_->SuspendLayout();
      this->SuspendLayout();
      // 
      // picture_box_panel_
      // 
      this->picture_box_panel_->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
        | System::Windows::Forms::AnchorStyles::Left)
        | System::Windows::Forms::AnchorStyles::Right));
      this->picture_box_panel_->AutoScroll = true;
      this->picture_box_panel_->Controls->Add(this->destination_picture_box_);
      this->picture_box_panel_->Controls->Add(this->source_picture_box_);
      this->picture_box_panel_->Location = System::Drawing::Point(69, 63);
      this->picture_box_panel_->Name = L"picture_box_panel_";
      this->picture_box_panel_->Size = System::Drawing::Size(1190, 495);
      this->picture_box_panel_->TabIndex = 0;
      // 
      // destination_picture_box_
      // 
      this->destination_picture_box_->Location = System::Drawing::Point(111, 4);
      this->destination_picture_box_->Name = L"destination_picture_box_";
      this->destination_picture_box_->Size = System::Drawing::Size(100, 50);
      this->destination_picture_box_->SizeMode = System::Windows::Forms::PictureBoxSizeMode::AutoSize;
      this->destination_picture_box_->TabIndex = 1;
      this->destination_picture_box_->TabStop = false;
      // 
      // source_picture_box_
      // 
      this->source_picture_box_->Location = System::Drawing::Point(4, 4);
      this->source_picture_box_->Name = L"source_picture_box_";
      this->source_picture_box_->Size = System::Drawing::Size(100, 50);
      this->source_picture_box_->SizeMode = System::Windows::Forms::PictureBoxSizeMode::AutoSize;
      this->source_picture_box_->TabIndex = 0;
      this->source_picture_box_->TabStop = false;
      // 
      // files_menu_strip_
      // 
      this->files_menu_strip_->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {
        this->files_tool_strip_menu_item_
      });
      this->files_menu_strip_->Location = System::Drawing::Point(0, 0);
      this->files_menu_strip_->Name = L"files_menu_strip_";
      this->files_menu_strip_->Size = System::Drawing::Size(1271, 24);
      this->files_menu_strip_->TabIndex = 1;
      this->files_menu_strip_->Text = L"Menu";
      // 
      // files_tool_strip_menu_item_
      // 
      this->files_tool_strip_menu_item_->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
        this->open_source_image_tool_strip_menu_item_,
          this->open_destination_image_tool_strip_menu_item_
      });
      this->files_tool_strip_menu_item_->Name = L"files_tool_strip_menu_item_";
      this->files_tool_strip_menu_item_->Size = System::Drawing::Size(42, 20);
      this->files_tool_strip_menu_item_->Text = L"Files";
      // 
      // open_source_image_tool_strip_menu_item_
      // 
      this->open_source_image_tool_strip_menu_item_->Name = L"open_source_image_tool_strip_menu_item_";
      this->open_source_image_tool_strip_menu_item_->Size = System::Drawing::Size(201, 22);
      this->open_source_image_tool_strip_menu_item_->Text = L"Open source image";
      // 
      // open_destination_image_tool_strip_menu_item_
      // 
      this->open_destination_image_tool_strip_menu_item_->Name = L"open_destination_image_tool_strip_menu_item_";
      this->open_destination_image_tool_strip_menu_item_->Size = System::Drawing::Size(201, 22);
      this->open_destination_image_tool_strip_menu_item_->Text = L"Open destination image";
      // 
      // ApplicationForm
      // 
      this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
      this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
      this->ClientSize = System::Drawing::Size(1271, 570);
      this->Controls->Add(this->picture_box_panel_);
      this->Controls->Add(this->files_menu_strip_);
      this->MainMenuStrip = this->files_menu_strip_;
      this->Name = L"ApplicationForm";
      this->Text = L"ApplicationForm";
      this->picture_box_panel_->ResumeLayout(false);
      this->picture_box_panel_->PerformLayout();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->destination_picture_box_))->EndInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->source_picture_box_))->EndInit();
      this->files_menu_strip_->ResumeLayout(false);
      this->files_menu_strip_->PerformLayout();
      this->ResumeLayout(false);
      this->PerformLayout();

    }
#pragma endregion
  };
}
