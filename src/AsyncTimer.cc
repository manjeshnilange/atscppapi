/*
 * Copyright (c) 2013 LinkedIn Corp. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the license at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 */

/**
 * @file AsyncTimer.cc
 * @author Brian Geffon
 * @author Manjesh Nilange
 */
#include "atscppapi/AsyncTimer.h"
#include <ts/ts.h>
#include "logging_internal.h"

using namespace atscppapi;

struct atscppapi::AsyncTimerState {
  TSCont cont_;
  AsyncTimer::Type type_;
  int period_in_ms_;
  int initial_period_in_ms_;
  TSAction initial_timer_action_;
  TSAction periodic_timer_action_;
  AsyncTimer *timer_;
  shared_ptr<AsyncDispatchControllerBase> dispatch_controller_;
  AsyncTimerState(AsyncTimer::Type type, int period_in_ms, int initial_period_in_ms, AsyncTimer *timer)
    : type_(type), period_in_ms_(period_in_ms), initial_period_in_ms_(initial_period_in_ms),
      initial_timer_action_(NULL), periodic_timer_action_(NULL), timer_(timer) { }
};

namespace {

int handleTimerEvent(TSCont cont, TSEvent event, void *edata) {
  AsyncTimerState *state = static_cast<AsyncTimerState *>(TSContDataGet(cont));
  if (state->initial_timer_action_) {
    LOG_DEBUG("Received initial timer event.");
    state->initial_timer_action_ = NULL; // mark it so that it won't be canceled in the destructor
    if (state->type_ == AsyncTimer::TYPE_PERIODIC) {
      LOG_DEBUG("Scheduling periodic event now");
      state->periodic_timer_action_ = TSContScheduleEvery(state->cont_, state->period_in_ms_,
                                                          TS_THREAD_POOL_DEFAULT);
    }
  }
  if (!state->dispatch_controller_->dispatch()) {
    LOG_DEBUG("Receiver has died. Destroying timer");
    delete state->timer_; // auto-destruct only in this case
  }
  return 0;
}

}

AsyncTimer::AsyncTimer(Type type, int period_in_ms, int initial_period_in_ms) {
  state_ = new AsyncTimerState(type, period_in_ms, initial_period_in_ms, this);
  TSMutex null_mutex = NULL;
  state_->cont_ = TSContCreate(handleTimerEvent, null_mutex);
  TSContDataSet(state_->cont_, static_cast<void *>(state_));
}

void AsyncTimer::run(shared_ptr<AsyncDispatchControllerBase> dispatch_controller) {
  int one_off_timeout_in_ms = 0;
  int regular_timeout_in_ms = 0;
  if (state_->type_ == AsyncTimer::TYPE_ONE_OFF) {
    one_off_timeout_in_ms = state_->period_in_ms_;
  }
  else {
    one_off_timeout_in_ms = state_->initial_period_in_ms_;
    regular_timeout_in_ms = state_->period_in_ms_;
  }
  if (one_off_timeout_in_ms) {
    LOG_DEBUG("Scheduling initial/one-off event");
    state_->initial_timer_action_ = TSContSchedule(state_->cont_, one_off_timeout_in_ms,
                                                   TS_THREAD_POOL_DEFAULT);
  }
  else if (regular_timeout_in_ms) {
    LOG_DEBUG("Scheduling regular timer events");
    state_->periodic_timer_action_ = TSContScheduleEvery(state_->cont_, regular_timeout_in_ms,
                                                         TS_THREAD_POOL_DEFAULT);
  }
  state_->dispatch_controller_ = dispatch_controller;
}

AsyncTimer::~AsyncTimer() {
  if (state_->initial_timer_action_) {
    LOG_DEBUG("Canceling initial timer action");
    TSActionCancel(state_->initial_timer_action_);
  }
  if (state_->periodic_timer_action_) {
    LOG_DEBUG("Canceling periodic timer action");
    TSActionCancel(state_->periodic_timer_action_);
  }
  LOG_DEBUG("Destroying cont");
  TSContDestroy(state_->cont_);
  delete state_;
}
