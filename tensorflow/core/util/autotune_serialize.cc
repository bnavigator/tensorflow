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

// For Google-internal use only.
#include "tensorflow/core/util/autotune_serialize.h"

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tensorflow/core/util/activation_mode.h"
#include "tensorflow/core/util/autotune_map.pb.h"
#include "tensorflow/core/util/autotune_maps/autotune_maps_utils.h"
#include "tensorflow/core/util/autotune_maps/conv_parameters.h"
#include "tensorflow/core/util/autotune_maps/conv_parameters.pb.h"
#include "tensorflow/core/util/autotune_maps/fused_conv_bias_activation_autotune_map.h"
#include "tensorflow/stream_executor/dnn.h"
#include "tensorflow/stream_executor/dnn.pb.h"

namespace tensorflow {

#if GOOGLE_CUDA || TENSORFLOW_USE_ROCM
namespace {
using stream_executor::dnn::AlgorithmConfig;
using stream_executor::dnn::AlgorithmConfigProto;
using stream_executor::dnn::AlgorithmDesc;
using stream_executor::dnn::AlgorithmProto;

ConvBiasActivationMapProto ConvBiasActivationMapToProto() {
  ConvBiasActivationMapProto proto;

  // Deterministically sort the entries in autotune maps
  // according to the serialized string of ConvParametersProto in order to
  // enable deterministic serialization. The actual order is meaningless.
  //

  // This step also filters out dupilicate entries (only device_id's are
  // different) in the autotune maps. So that there is only one entry for a
  // convolution operation with a specific GPU device type.
  std::map<string, ConvBiasActivationMapProto::Entry> sorted_map;

  for (auto const &p : AutotuneConvBiasActivation::GetInstance()->GetMap()) {
    const AlgorithmConfig &config = p.second;
    // Skip entries that use cuDNN Frontend API because currently they cannot be
    // serialized.
    if (config.algorithm().value().IsExecutionPlan()) {
      continue;
    }
    const ConvParameters &params = p.first;
    const ConvParametersProto &params_proto = params.proto();

    ConvBiasActivationMapProto::Entry kv;
    VLOG(1) << "Reading: " << p.first.ToString();
    *kv.mutable_key() = params_proto;
    *kv.mutable_value() = config.ToProto();
    sorted_map.insert(std::make_pair(
        autotune_maps_utils::SerializeProtoDeterministic(params_proto), kv));
  }

  for (auto const &p : sorted_map) {
    ConvBiasActivationMapProto::Entry *kv = proto.add_kv_pairs();
    *kv = p.second;
  }
  return proto;
}

void PopulateConvBiasActivationMap(const ConvBiasActivationMapProto &m) {
  // Map device_id's to corresponding device_identifiers.
  std::vector<string> device_ids_map =
      autotune_maps_utils::GetDeviceIdToIdentifierMap();
  // Map device_identifiers to device_ids whose corresponding GPU devices have
  // the given device_identifier.
  std::unordered_map<string, std::vector<int>> device_identifiers_map;

  for (const ConvBiasActivationMapProto::Entry &kv : m.kv_pairs()) {
    const ConvParametersProto &params_proto = kv.key();
    const AlgorithmConfigProto &algorithm_config_proto = kv.value();
    auto iter = device_identifiers_map.find(params_proto.device_identifier());
    std::vector<int> device_ids;
    if (iter == device_identifiers_map.end()) {
      for (int i = 0; i < device_ids_map.size(); i++) {
        if (device_ids_map[i] == params_proto.device_identifier()) {
          device_ids.push_back(i);
        }
      }
      device_identifiers_map.insert(
          std::make_pair(params_proto.device_identifier(), device_ids));
    } else {
      device_ids = iter->second;
    }
    for (int device_id : device_ids) {
      AutotuneConvBiasActivation::GetInstance()->Insert(
          ConvParameters(device_id, params_proto),
          AlgorithmConfig(algorithm_config_proto));
    }
  }
}

}  // namespace
#endif  // GOOGLE_CUDA || TENSORFLOW_USE_ROCM

Status SerializeAutotuneMaps(std::string *output) {
  AutotuneMapsProto proto;
#if GOOGLE_CUDA || TENSORFLOW_USE_ROCM
  *proto.mutable_conv_bias_activation_map() = ConvBiasActivationMapToProto();
#endif  // GOOGLE_CUDA || TENSORFLOW_USE_ROCM
  *output = autotune_maps_utils::SerializeProtoDeterministic(proto);
  return Status::OK();
}

Status LoadSerializedAutotuneMaps(absl::string_view s) {
#if GOOGLE_CUDA || TENSORFLOW_USE_ROCM
  AutotuneMapsProto proto;
  // The explicit string conversion here is a workaround for
  // resolving the issue that OSS proto library's ParseFromString only accepts
  // std::string.
  if (!proto.ParseFromString(string(s))) {
    return errors::InvalidArgument(
        "Failed to parse the autotune maps from string.");
  }

  PopulateConvBiasActivationMap(proto.conv_bias_activation_map());
  // TODO(b/189530096) Populate autotune maps for more ops.
#endif  // GOOGLE_CUDA || TENSORFLOW_USE_ROCM
  return Status::OK();
}

}  // namespace tensorflow
