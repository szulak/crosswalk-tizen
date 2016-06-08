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

#ifndef XWALK_RUNTIME_BROWSER_IME_APPLICATION_H_
#define XWALK_RUNTIME_BROWSER_IME_APPLICATION_H_

#include <functional>
#include <list>
#include <memory>
#include <string>

#include "runtime/browser/web_view.h"
#include "runtime/browser/native_window.h"
#include "runtime/browser/web_application.h"

class Ewk_Context;

namespace runtime {

class ImeApplication : public WebApplication {
 public:
  ImeApplication(
    NativeWindow* window, common::ApplicationData* app_data,
    Ewk_Context* context);
  virtual ~ImeApplication();
 protected:
  virtual void OnLoadFinished(WebView* view);  // override
};

}  // namespace runtime

#endif  // XWALK_RUNTIME_BROWSER_IME_APPLICATION_H_
