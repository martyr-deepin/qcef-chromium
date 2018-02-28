// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/security_state_tab_helper.h"

#include "base/command_line.h"
#include "base/test/histogram_tester.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/security_state/core/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kHTTPBadNavigationHistogram[] =
    "Security.HTTPBad.NavigationStartedAfterUserWarnedAboutSensitiveInput";
const char kHTTPBadWebContentsDestroyedHistogram[] =
    "Security.HTTPBad.WebContentsDestroyedAfterUserWarnedAboutSensitiveInput";

class SecurityStateTabHelperHistogramTest
    : public ChromeRenderViewHostTestHarness,
      public testing::WithParamInterface<bool> {
 public:
  SecurityStateTabHelperHistogramTest() : helper_(nullptr) {}
  ~SecurityStateTabHelperHistogramTest() override {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    SecurityStateTabHelper::CreateForWebContents(web_contents());
    helper_ = SecurityStateTabHelper::FromWebContents(web_contents());
    NavigateToHTTP();
  }

 protected:
  void SignalSensitiveInput() {
    if (GetParam())
      web_contents()->OnPasswordInputShownOnHttp();
    else
      web_contents()->OnCreditCardInputShownOnHttp();
    helper_->VisibleSecurityStateChanged();
  }

  const std::string HistogramName() {
    if (GetParam())
      return "Security.HTTPBad.UserWarnedAboutSensitiveInput.Password";
    else
      return "Security.HTTPBad.UserWarnedAboutSensitiveInput.CreditCard";
  }

  void NavigateToHTTP() { NavigateAndCommit(GURL("http://example.test")); }

  void NavigateToDifferentHTTPPage() {
    NavigateAndCommit(GURL("http://example2.test"));
  }

 private:
  SecurityStateTabHelper* helper_;
  DISALLOW_COPY_AND_ASSIGN(SecurityStateTabHelperHistogramTest);
};

// Tests that an UMA histogram is recorded after setting the security
// level to HTTP_SHOW_WARNING and navigating away.
TEST_P(SecurityStateTabHelperHistogramTest,
       HTTPOmniboxWarningNavigationHistogram) {
  base::HistogramTester histograms;
  SignalSensitiveInput();
  // Make sure that if the omnibox warning gets dynamically hidden, the
  // histogram still gets recorded.
  NavigateToDifferentHTTPPage();
  if (GetParam())
    web_contents()->OnAllPasswordInputsHiddenOnHttp();
  // Destroy the WebContents to simulate the tab being closed after a
  // navigation.
  SetContents(nullptr);
  histograms.ExpectTotalCount(kHTTPBadNavigationHistogram, 1);
  histograms.ExpectTotalCount(kHTTPBadWebContentsDestroyedHistogram, 0);
}

// Tests that an UMA histogram is recorded after setting the security
// level to HTTP_SHOW_WARNING and closing the tab.
TEST_P(SecurityStateTabHelperHistogramTest,
       HTTPOmniboxWarningTabClosedHistogram) {
  base::HistogramTester histograms;
  SignalSensitiveInput();
  // Destroy the WebContents to simulate the tab being closed.
  SetContents(nullptr);
  histograms.ExpectTotalCount(kHTTPBadNavigationHistogram, 0);
  histograms.ExpectTotalCount(kHTTPBadWebContentsDestroyedHistogram, 1);
}

// Tests that UMA logs the omnibox warning when security level is
// HTTP_SHOW_WARNING.
TEST_P(SecurityStateTabHelperHistogramTest, HTTPOmniboxWarningHistogram) {
  base::HistogramTester histograms;
  SignalSensitiveInput();
  histograms.ExpectUniqueSample(HistogramName(), true, 1);

  // Fire again and ensure no sample is recorded.
  SignalSensitiveInput();
  histograms.ExpectUniqueSample(HistogramName(), true, 1);

  // Navigate to a new page and ensure a sample is recorded.
  NavigateToDifferentHTTPPage();
  histograms.ExpectUniqueSample(HistogramName(), true, 1);
  SignalSensitiveInput();
  histograms.ExpectUniqueSample(HistogramName(), true, 2);
}

INSTANTIATE_TEST_CASE_P(SecurityStateTabHelperHistogramTest,
                        SecurityStateTabHelperHistogramTest,
                        // Here 'true' to test password field triggered
                        // histogram and 'false' to test credit card field.
                        testing::Bool());

}  // namespace