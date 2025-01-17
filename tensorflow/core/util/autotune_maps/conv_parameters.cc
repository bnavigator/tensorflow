/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if GOOGLE_CUDA || TENSORFLOW_USE_ROCM
#include "tensorflow/core/util/autotune_maps/conv_parameters.h"

#include <vector>

#include "absl/strings/str_format.h"
#include "tensorflow/core/platform/hash.h"
#include "tensorflow/core/util/autotune_maps/autotune_maps_utils.h"
#include "tensorflow/core/util/autotune_maps/conv_parameters.pb.h"

namespace tensorflow {

namespace {
using ::tensorflow::protobuf::util::MessageDifferencer;

uint64 ComputeHash(int device_id, const ConvParametersProto& proto) {
  return Hash64Combine(device_id, autotune_maps_utils::HashProto(proto));
}
}  // namespace

ConvParameters::ConvParameters(
    int64_t batch, int64_t in_depths, const absl::Span<const int64_t> in,
    int data_format, int64_t out_depths, const absl::Span<const int64_t> filter,
    const absl::Span<const int64_t> dilation,
    const absl::Span<const int64_t> stride,
    const absl::Span<const int64_t> padding, DataType dtype, int device_id,
    int group_count, bool has_side_input,
    stream_executor::dnn::ActivationMode activation_mode)
    : device_id_(device_id) {
  proto_.set_batch(batch);
  proto_.set_in_depths(in_depths);
  *proto_.mutable_in() = {in.begin(), in.end()};
  proto_.set_data_format(static_cast<int>(data_format));
  proto_.set_out_depths(out_depths);
  *proto_.mutable_filter() = {filter.begin(), filter.end()};
  *proto_.mutable_dilation() = {dilation.begin(), dilation.end()};
  *proto_.mutable_stride() = {stride.begin(), stride.end()};
  *proto_.mutable_padding() = {padding.begin(), padding.end()};
  proto_.set_dtype(dtype);
  proto_.set_group_count(group_count);
  proto_.mutable_fusion()->set_has_side_input(has_side_input);
  proto_.mutable_fusion()->set_activation_mode(activation_mode);
  proto_.set_device_identifier(
      autotune_maps_utils::DeviceIdToIdentifier(device_id));
  hash_code_ = ComputeHash(device_id_, proto_);
}

ConvParameters::ConvParameters(int device_id, const ConvParametersProto& proto)
    : device_id_(device_id),
      proto_(proto),
      hash_code_(ComputeHash(device_id, proto_)) {}

bool ConvParameters::operator==(const ConvParameters& other) const {
  return device_id_ == other.device_id_ &&
         MessageDifferencer::Equals(this->proto_, other.proto_);
}

string ConvParameters::ToString() const { return proto_.DebugString(); }

}  // namespace tensorflow

#endif  // GOOGLE_CUDA || TENSORFLOW_USE_ROCM
