/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *    This file has been modified by Silicon Labs.
 *    =======================================================================*/
#include <cstdio>
#include <limits>

#include "recognize_commands.h"

RecognizeCommands::RecognizeCommands(tflite::ErrorReporter* error_reporter,
                                     int32_t average_window_duration_ms,
                                     uint8_t detection_threshold,
                                     int32_t suppression_count,
                                     int32_t minimum_count)
  : error_reporter_(error_reporter),
    average_window_duration_ms_(average_window_duration_ms),
    detection_threshold_(detection_threshold),
    suppression_count_(suppression_count),
    minimum_count_(minimum_count),
    previous_results_(error_reporter) {
  previous_top_label_index_ = 0;
  previous_top_label_time_ = 0;
  suppression_remaining_ = 0;
}

TfLiteStatus RecognizeCommands::ProcessLatestResults(
  const TfLiteTensor* latest_results, const int32_t current_time_ms,
  uint8_t* found_command_index, uint8_t* score, bool* is_new_command)
{
  if ((!previous_results_.empty())
      && (current_time_ms < previous_results_.front().time_)) {
      TF_LITE_REPORT_ERROR(
      error_reporter_,
      "Results must be fed in increasing time order, but received a "
      "timestamp of %d that was earlier than the previous one of %d",
      current_time_ms, previous_results_.front().time_);
    return kTfLiteError;
  }

  // Add the latest results to the head of the queue.
  uint8_t converted_scores[TFLITE_MODEL_CLASS_COUNT];

  // Convert the model output from float32 to uint8
  if (latest_results->type == kTfLiteFloat32) {
    for(int i = 0; i < TFLITE_MODEL_CLASS_COUNT; ++i) {
      converted_scores[i] = (uint8_t)(latest_results->data.f[i] *255);
    }
  } else {
    TF_LITE_REPORT_ERROR(
          error_reporter_,
          "Unsupported output tensor data type, must be float32");
    return kTfLiteError;
  }

  previous_results_.push_back({current_time_ms, converted_scores});

  // Prune any earlier results that are too old for the averaging window.
  const int64_t time_limit = current_time_ms - average_window_duration_ms_;
  while ((!previous_results_.empty())
         && previous_results_.front().time_ < time_limit) {
    previous_results_.pop_front();
  }

  // If there are too few results, assume the result will be unreliable and
  // bail.
  static int consecutive_min_count = 0;
  const int32_t how_many_results = previous_results_.size();
  if ((how_many_results < minimum_count_)) {
    ++consecutive_min_count;
    if(consecutive_min_count % 10 == 0) {
      TF_LITE_REPORT_ERROR(
        error_reporter_,
        "Too few samples for averaging. "
        "This likely means the inference loop is taking too long."
        "Either decrease the 'minimum_count' "
        "and/or increase 'average_window_duration_ms'\n");
    }
    *found_command_index = previous_top_label_index_;
    *score = 0;
    *is_new_command = false;
    return kTfLiteOk;
  }
  consecutive_min_count = 0;

  // Calculate the average score across all the results in the window.
  uint32_t average_scores[TFLITE_MODEL_CLASS_COUNT];
  for (int offset = 0; offset < previous_results_.size(); ++offset) {
    // Iterates the amount of times to achieve average_window_duration
    PreviousResultsQueue::Result previous_result =
      previous_results_.from_front(offset);
    const uint8_t* scores = previous_result.scores;
    for (int i = 0; i < TFLITE_MODEL_CLASS_COUNT; ++i) {
      if (offset == 0) {
        average_scores[i] = scores[i];
      } else {
        average_scores[i] += scores[i];
      }
    }
  }

  for (int i = 0; i < TFLITE_MODEL_CLASS_COUNT; ++i) {
    average_scores[i] /= how_many_results;
  }

  // Find the current highest scoring category.
  int8_t current_top_index = 0;
  uint32_t current_top_score = 0;
  for (int i = 0; i < TFLITE_MODEL_CLASS_COUNT; ++i) {
    if (average_scores[i] > current_top_score) {
      current_top_score = average_scores[i];
      current_top_index = i;
    }
  }

  // If we've recently had another label trigger, assume one that occurs too
  // soon afterwards is a bad result.
  *is_new_command = false;
  if(current_top_score >= detection_threshold_) {
    if(suppression_remaining_ > 0) {
      if(current_top_index != previous_top_label_index_) {
        --suppression_remaining_;
      }
    } else {
      suppression_remaining_ = suppression_count_;
      previous_top_label_index_ = current_top_index;
      previous_top_label_time_ = current_time_ms;
      *is_new_command = true;
    }
  }

  *found_command_index = current_top_index;
  *score = current_top_score;

  return kTfLiteOk;
}
