#include <opencv2/core/core.hpp>

#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>

#include "caffe/data_layers.hpp"
#include "caffe/layer.hpp"
#include "caffe/util/benchmark.hpp"
#include "caffe/util/io.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/util/rng.hpp"

namespace caffe {
//TODO: Jackie
//Desc: Construct a new class "MyImageDataLayer" definition
template <typename Dtype>
MyImageDataLayer<Dtype>::~MyImageDataLayer<Dtype>() {
  this->JoinPrefetchThread();
}

template <typename Dtype>
void MyImageDataLayer<Dtype>::DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  //TODO: Jackie
  //Desc: Change all into my_image_data_param defined in caffe.proto
  const int new_height = this->layer_param_.my_image_data_param().new_height();
  const int new_width  = this->layer_param_.my_image_data_param().new_width();
  const bool is_color  = this->layer_param_.my_image_data_param().is_color();
  string root_folder = this->layer_param_.my_image_data_param().root_folder();

  CHECK((new_height == 0 && new_width == 0) ||
      (new_height > 0 && new_width > 0)) << "Current implementation requires "
      "new_height and new_width to be set at the same time.";
  // Read the file with filenames and labels
  const string& source = this->layer_param_.my_image_data_param().source();
  LOG(INFO) << "Opening file " << source;
  std::ifstream infile(source.c_str());
  string filename;

  /*
  int label;
  while (infile >> filename >> label) {
    lines_.push_back(std::make_pair(filename, label));
  }
  */

  //TODO: Jackie
  //Desc: Get label_num from caffe.proto defined and change input for multi-label inputed
  const int label_num = this->layer_param_.my_image_data_param().label_num();
  int label;
  vector<int> labels;
  while (infile >> filename) {
    labels.clear();
    for (int i = 0; i < label_num; i++) {
        infile >> label;
        labels.push_back(label);
    }
    lines_.push_back(std::make_pair(filename, labels));
  }

  if (this->layer_param_.my_image_data_param().shuffle()) {
    // randomly shuffle data
    LOG(INFO) << "Shuffling data";
    const unsigned int prefetch_rng_seed = caffe_rng_rand();
    prefetch_rng_.reset(new Caffe::RNG(prefetch_rng_seed));
    ShuffleImages();
  }
  LOG(INFO) << "A total of " << lines_.size() << " images.";

  lines_id_ = 0;
  // Check if we would need to randomly skip a few data points
  if (this->layer_param_.my_image_data_param().rand_skip()) {
    unsigned int skip = caffe_rng_rand() %
        this->layer_param_.my_image_data_param().rand_skip();
    LOG(INFO) << "Skipping first " << skip << " data points.";
    CHECK_GT(lines_.size(), skip) << "Not enough points to skip";
    lines_id_ = skip;
  }
  // Read an image, and use it to initialize the top blob.
  cv::Mat cv_img = ReadImageToCVMat(root_folder + lines_[lines_id_].first,
                                    new_height, new_width, is_color);
  //TODO: Jackie
  // Use data_transformer to infer the expected blob shape from a cv_image.
  vector<int> top_shape = this->data_transformer_->InferBlobShape(cv_img);
  this->transformed_data_.Reshape(top_shape);
  // Reshape prefetch_data and top[0] according to the batch_size.
  const int batch_size = this->layer_param_.my_image_data_param().batch_size();
  top_shape[0] = batch_size;
  this->prefetch_data_.Reshape(top_shape);
  top[0]->ReshapeLike(this->prefetch_data_);

  LOG(INFO) << "output data size: " << top[0]->num() << ","
      << top[0]->channels() << "," << top[0]->height() << ","
      << top[0]->width();
  // label
  //vector<int> label_shape(1, batch_size);
  
  //TODO: Jackie
  //Desc: push back label_num after batch_size in order to reshape the blob
  vector<int> label_shape;
  label_shape.push_back(batch_size);
  label_shape.push_back(label_num);

  top[1]->Reshape(label_shape);
  this->prefetch_label_.Reshape(label_shape);
}

template <typename Dtype>
void MyImageDataLayer<Dtype>::ShuffleImages() {
  caffe::rng_t* prefetch_rng =
      static_cast<caffe::rng_t*>(prefetch_rng_->generator());
  shuffle(lines_.begin(), lines_.end(), prefetch_rng);
}

// This function is used to create a thread that prefetches the data.
template <typename Dtype>
void MyImageDataLayer<Dtype>::InternalThreadEntry() {
  CPUTimer batch_timer;
  batch_timer.Start();
  double read_time = 0;
  double trans_time = 0;
  CPUTimer timer;
  CHECK(this->prefetch_data_.count());
  CHECK(this->transformed_data_.count());
  MyImageDataParameter my_image_data_param = this->layer_param_.my_image_data_param();
  const int batch_size = my_image_data_param.batch_size();
  const int new_height = my_image_data_param.new_height();
  const int new_width = my_image_data_param.new_width();
  const bool is_color = my_image_data_param.is_color();
  string root_folder = my_image_data_param.root_folder();

  // Reshape according to the first image of each batch
  // on single input batches allows for inputs of varying dimension.
  cv::Mat cv_img = ReadImageToCVMat(root_folder + lines_[lines_id_].first,
      new_height, new_width, is_color);
  //TODO: Jackie
  //Desc: Fix bug when picture is broken or no-exist
  while (cv_img.data == NULL) {
      ++lines_id_;
      const int lines_size = lines_.size();
      if (lines_id_ >= lines_size) {
          // We have reached the end. Restart from the first.
          DLOG(INFO) << "Restarting data prefetching from start.";
          lines_id_ = 0;
          if (this->layer_param_.my_image_data_param().shuffle()) {
              ShuffleImages();
          }
        }
        cv::Mat cv_img = ReadImageToCVMat(root_folder + lines_[lines_id_].first,
            new_height, new_width, is_color);
  }
  // Use data_transformer to infer the expected blob shape from a cv_img.
  vector<int> top_shape = this->data_transformer_->InferBlobShape(cv_img);
  this->transformed_data_.Reshape(top_shape);
  // Reshape prefetch_data according to the batch_size.
  top_shape[0] = batch_size;
  this->prefetch_data_.Reshape(top_shape);

  Dtype* prefetch_data = this->prefetch_data_.mutable_cpu_data();
  Dtype* prefetch_label = this->prefetch_label_.mutable_cpu_data();

  // datum scales
  const int lines_size = lines_.size();
  for (int item_id = 0; item_id < batch_size; ++item_id) {
    // get a blob
    timer.Start();
    CHECK_GT(lines_size, lines_id_);
    cv::Mat cv_img = ReadImageToCVMat(root_folder + lines_[lines_id_].first,
        new_height, new_width, is_color);
    //TODO: Jackie
    //Desc: Fix bug when picture is broken or no-exist
    if (cv_img.data == NULL) {
        --item_id;
        lines_id_++;
        if (lines_id_ >= lines_size) {
          // We have reached the end. Restart from the first.
          DLOG(INFO) << "Restarting data prefetching from start.";
          lines_id_ = 0;
          if (this->layer_param_.my_image_data_param().shuffle()) {
            ShuffleImages();
          }
        }
        continue;
    }
    //CHECK(cv_img.data) << "Could not load " << lines_[lines_id_].first;
    read_time += timer.MicroSeconds();
    timer.Start();
    // Apply transformations (mirror, crop...) to the image
    int offset = this->prefetch_data_.offset(item_id);
    this->transformed_data_.set_cpu_data(prefetch_data + offset);
    this->data_transformer_->Transform(cv_img, &(this->transformed_data_));
    trans_time += timer.MicroSeconds();

    //prefetch_label[item_id] = lines_[lines_id_].second;
    
    //TODO: Jackie
    //Desc: full the prefetch_label by inputed label info
    const int label_num = this->layer_param_.my_image_data_param().label_num();
    for (int i = 0; i < label_num; i++) {
        prefetch_label[item_id * label_num + i] = lines_[lines_id_].second[i];
    }

    // go to the next iter
    lines_id_++;
    if (lines_id_ >= lines_size) {
      // We have reached the end. Restart from the first.
      DLOG(INFO) << "Restarting data prefetching from start.";
      lines_id_ = 0;
      if (this->layer_param_.my_image_data_param().shuffle()) {
        ShuffleImages();
      }
    }
  }
  batch_timer.Stop();
  DLOG(INFO) << "Prefetch batch: " << batch_timer.MilliSeconds() << " ms.";
  DLOG(INFO) << "     Read time: " << read_time / 1000 << " ms.";
  DLOG(INFO) << "Transform time: " << trans_time / 1000 << " ms.";
}

//TODO: Jackie
//Desc: Register the Class and the type
INSTANTIATE_CLASS(MyImageDataLayer);
REGISTER_LAYER_CLASS(MyImageData);

}  // namespace caffe
