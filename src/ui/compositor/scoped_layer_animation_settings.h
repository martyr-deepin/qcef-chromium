// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_SCOPED_LAYER_ANIMATION_SETTINGS_H_
#define UI_COMPOSITOR_SCOPED_LAYER_ANIMATION_SETTINGS_H_

#include <set>

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/compositor/compositor_export.h"
#include "ui/compositor/layer_animation_element.h"
#include "ui/compositor/layer_animator.h"
#include "ui/gfx/animation/tween.h"

namespace ui {

class ImplicitAnimationObserver;

// Scoped settings allow you to temporarily change the animator's settings and
// these changes are reverted when the object is destroyed. NOTE: when the
// settings object is created, it applies the default transition duration
// (200ms).
class COMPOSITOR_EXPORT ScopedLayerAnimationSettings {
 public:
  explicit ScopedLayerAnimationSettings(scoped_refptr<LayerAnimator> animator);
  virtual ~ScopedLayerAnimationSettings();

  void AddObserver(ImplicitAnimationObserver* observer);

  void SetAnimationMetricsReporter(AnimationMetricsReporter* reporter);
  void SetTransitionDuration(base::TimeDelta duration);
  base::TimeDelta GetTransitionDuration() const;

  // Locks transition duration in |animator_|. When transition duration
  // is locked any subsequent changes to it are ignored until the
  // ScopedLayerAnimationSettings object that has locked the duration goes out
  // of scope.
  void LockTransitionDuration();

  void SetTweenType(gfx::Tween::Type tween_type);
  gfx::Tween::Type GetTweenType() const;

  void SetPreemptionStrategy(LayerAnimator::PreemptionStrategy strategy);
  LayerAnimator::PreemptionStrategy GetPreemptionStrategy() const;

 private:
  scoped_refptr<LayerAnimator> animator_;
  bool old_is_transition_duration_locked_;
  base::TimeDelta old_transition_duration_;
  gfx::Tween::Type old_tween_type_;
  LayerAnimator::PreemptionStrategy old_preemption_strategy_;
  std::set<ImplicitAnimationObserver*> observers_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLayerAnimationSettings);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_SCOPED_LAYER_ANIMATION_SETTINGS_H_