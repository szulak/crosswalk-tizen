/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "runtime/browser/splash_screen.h"

#include <map>

#include <string>
#include <vector>

#if defined(HAVE_X11)
#include <Ecore_X.h>
#elif defined(HAVE_WAYLAND)
#include <Ecore_Wayland.h>
#endif

#include "common/logger.h"
#include "manifest_handlers/splash_screen_handler.h"
#include "runtime/browser/native_window.h"

using ScreenOrientation = runtime::NativeWindow::ScreenOrientation;

namespace {

wgt::parse::ScreenOrientation ChooseOrientation(
    const std::map<wgt::parse::ScreenOrientation,
    wgt::parse::SplashScreenData>& splash_map,
    ScreenOrientation screen_orientation) {
  auto orientation_pair = splash_map.end();

  if (screen_orientation ==
      runtime::NativeWindow::ScreenOrientation::PORTRAIT_PRIMARY) {
     orientation_pair =
         splash_map.find(wgt::parse::ScreenOrientation::PORTRAIT);
  } else {
    orientation_pair =
        splash_map.find(wgt::parse::ScreenOrientation::LANDSCAPE);
  }
  if (orientation_pair == splash_map.end())
        orientation_pair = splash_map.find(wgt::parse::ScreenOrientation::AUTO);

  if (orientation_pair != splash_map.end()) return orientation_pair->first;
  return wgt::parse::ScreenOrientation::NONE;
}

void SetImageBorder(Evas_Object* image,
                    const runtime::SplashScreen::SplashScreenBound& bound,
                    const std::vector<std::string>& borders) {
  enum class BorderOption { REPEAT = 1, STRETCH, ROUND };
  std::map<std::string, BorderOption> scaling_string;
  scaling_string["repeat"] = BorderOption::REPEAT;
  scaling_string["stretch"] = BorderOption::STRETCH;
  scaling_string["round"] = BorderOption::ROUND;

  std::vector<int> border_values;
  std::vector<BorderOption> border_options;

  for (const auto& border : borders) {
    std::string::size_type px_index = border.find("px");
    if (px_index != std::string::npos) {
      std::string border_value(border.begin(), border.begin() + px_index);
      border_values.push_back(std::atoi(border_value.c_str()));
    }
    for (const auto& border_string_val : scaling_string) {
      std::string::size_type index = border.find(border_string_val.first);
      if (index != std::string::npos) {
        border_options.push_back(border_string_val.second);
      }
    }
  }

  LOGGER(DEBUG) << "Image border values:";
  for (const auto& border_value : border_values) {
    LOGGER(DEBUG) << border_value;
  }
  LOGGER(DEBUG) << "Image border scaling values:";
  for (const auto& border_option : border_options) {
    LOGGER(DEBUG) << static_cast<int>(border_option);
  }
  // TODO(a.szulakiewi): modify 'image' according to border and scaling type
}

}  // namespace

namespace runtime {

SplashScreen::SplashScreen(
    runtime::NativeWindow* window,
    std::shared_ptr<const wgt::parse::SplashScreenInfo> ss_info,
    const std::string& app_path)
    : window_(window),
      ss_info_(ss_info),
      image_(nullptr),
      background_(nullptr),
      is_active_(false) {
  LOGGER(DEBUG) << "start of create splash screen";
  if (ss_info == nullptr) return;
  auto splash_map = ss_info->splash_screen_data();
  auto used_orientation =
      ChooseOrientation(splash_map, window->natural_orientation());
  if (used_orientation == wgt::parse::ScreenOrientation::NONE) return;

  auto dimensions = GetDimensions();

  SetBackground(splash_map[used_orientation], window->evas_object(), dimensions,
           app_path);
  SetImage(splash_map[used_orientation], window->evas_object(), dimensions,
           app_path);
  is_active_ = true;
}


void SplashScreen::HideSplashScreen(HideReason reason) {
  if (!is_active_) return;
  if (reason == HideReason::RENDERED &&
      ss_info_->ready_when() != wgt::parse::ReadyWhen::FIRSTPAINT) {
    return;
  }
  if (reason == HideReason::LOADFINISHED &&
      ss_info_->ready_when() != wgt::parse::ReadyWhen::COMPLETE) {
    return;
  }

  evas_object_hide(background_);
  evas_object_hide(image_);
  evas_object_del(background_);
  evas_object_del(image_);
  background_ = nullptr;
  image_ = nullptr;
  is_active_ = false;
}

std::pair<int, int> SplashScreen::GetDimensions() {
  int w, h;
#if defined(HAVE_X11)
  uint16_t pid = getpid();
  ecore_x_window_prop_property_set(elm_win_xwindow_get(window_.evas_object()),
                                   ECORE_X_ATOM_NET_WM_PID,
                                   ECORE_X_ATOM_CARDINAL, 32, &pid, 1);
  ecore_x_vsync_animator_tick_source_set(
      elm_win_xwindow_get(window_.evas_object()));
  ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
#elif defined(HAVE_WAYLAND)
  ecore_wl_screen_size_get(&w, &h);
#endif
  evas_object_resize(background_, w, h);
  return std::make_pair(w, h);
}

void SplashScreen::SetBackground(
    wgt::parse::SplashScreenData& splash_data, Evas_Object* parent,
    const SplashScreenBound& bound, const std::string& app_path) {
  background_ = elm_bg_add(parent);
  if (!background_) return;
  evas_object_resize(background_, bound.first, bound.second);

  if (!splash_data.background_image.empty()) {
    const std::string& background_image_path =
        splash_data.background_image.front();
    elm_bg_file_set(background_,
                    (app_path + background_image_path).c_str(), NULL);
    elm_bg_option_set(background_, ELM_BG_OPTION_STRETCH);
  }

  if (splash_data.background_color != nullptr) {
    elm_bg_color_set(background_,
                     splash_data.background_color->red,
                     splash_data.background_color->green,
                     splash_data.background_color->blue);
  }
  evas_object_show(background_);
}

void SplashScreen::SetImage(const wgt::parse::SplashScreenData& splash_data,
                            Evas_Object* parent,
                            const SplashScreenBound& bound,
                            const std::string& app_path) {
  if (!background_) return;
  image_ = elm_image_add(background_);
  if (!image_) return;

  const std::string& image_path = splash_data.image.front();
  elm_image_file_set(image_, (app_path + image_path).c_str(), NULL);
  evas_object_resize(image_, bound.first, bound.second);
  evas_object_show(image_);
}
}  // namespace runtime
