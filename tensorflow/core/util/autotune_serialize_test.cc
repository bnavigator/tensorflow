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
#if GOOGLE_CUDA || TENSORFLOW_USE_ROCM

#include "tensorflow/core/util/autotune_serialize.h"

#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/util/autotune_maps/conv_parameters.h"
#include "tensorflow/core/util/autotune_maps/conv_parameters.pb.h"
#include "tensorflow/core/util/autotune_maps/fused_conv_bias_activation_autotune_map.h"
#include "tensorflow/core/util/tensor_format.h"
#include "tensorflow/stream_executor/gpu/gpu_driver.h"

namespace tensorflow {
namespace {
using stream_executor::dnn::AlgorithmConfig;
using stream_executor::dnn::AlgorithmDesc;
using stream_executor::gpu::GpuDriver;

// TODO(ruochengw): After merging autotune maps of Conv, FusedConv and
// FusedConvBiasActivation, update tests so that they test on the merged
// autotune map instead.

// Tests when there is no entry in the autotune maps.
TEST(AutotuneSerializeTest, Empty) {
  TF_CHECK_OK(GpuDriver::Init());
  AutotuneConvBiasActivation::GetInstance()->ClearMap();
  std::string output;
  TF_CHECK_OK(SerializeAutotuneMaps(&output));
  TF_CHECK_OK(LoadSerializedAutotuneMaps(output));
  EXPECT_EQ(AutotuneConvBiasActivation::GetInstance()->GetMap().size(), 0);
}

// Tests the consistency of SerializeAutotuneMaps and LoadSerializedAutotuneMaps
// by:
// 1. Insert predefined entries into the autotune maps.
// 2. Serialize it to string using SerializeAutotuneMaps.
// 3. Reset autotune maps.
// 4. Use MergeFromstring to load the entries from string to autotune maps.
// 5. Check if entries in autotune maps are equal to the predefined ones.
TEST(AutotuneSerializeTest, Consistency) {
  TF_CHECK_OK(GpuDriver::Init());
  AutotuneConvBiasActivation::GetInstance()->ClearMap();
  std::string serialized_string;
  ConvParameters params_example_a = {
      /*batch_size=*/1,
      /*in_depths=*/1,
      /*in=*/{{1, 1}},
      /*data_format=*/TensorFormat::FORMAT_NCHW,
      /*out_depth=*/1,
      /*filter=*/{{1, 1}},
      /*dilation=*/{{1, 1}},
      /*stride=*/{{1, 1}},
      /*padding=*/{{1, 1}},
      /*dtype=*/DataType::DT_INT8,
      /*device_id=*/0,
      /*group_count=*/1,
      /*has_side_input=*/true,
      /*activation_mode=*/se::dnn::ActivationMode::kRelu};
  AlgorithmDesc algorithm(/*algo_id=*/1, /*use_tensor_op=*/true);
  AlgorithmDesc algorithm_no_scratch(/*algo_id=*/1, /*use_tensor_op=*/true);
  AlgorithmConfig algorithm_config_example_a(algorithm, /*scratch_size=*/1,
                                             algorithm_no_scratch);
  AutotuneConvBiasActivation::GetInstance()->Insert(params_example_a,
                                                    algorithm_config_example_a);
  TF_CHECK_OK(SerializeAutotuneMaps(&serialized_string));
  AutotuneConvBiasActivation::GetInstance()->ClearMap();
  TF_CHECK_OK(LoadSerializedAutotuneMaps(serialized_string));
  EXPECT_EQ(AutotuneConvBiasActivation::GetInstance()->GetMap().size(), 1);
  AlgorithmConfig algorithm_config;
  EXPECT_TRUE(AutotuneConvBiasActivation::GetInstance()->Find(
      params_example_a, &algorithm_config));
  EXPECT_EQ(algorithm_config, algorithm_config_example_a);
}
}  // namespace
}  // namespace tensorflow
#endif  // GOOGLE_CUDA || TENSORFLOW_USE_ROCM
