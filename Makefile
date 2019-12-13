# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this file is governed by a BSD-style license that can be found at:
#   https://chromium.googlesource.com/chromiumos/platform2/+/master/LICENSE

include common.mk

CXX_BINARY(nvt-tcon-fw-updater): src/main.o src/NVTAuxCtrl.o src/NVTSettingProc.o
nvt-tcon-fw-updater: CXX_BINARY(nvt-tcon-fw-updater)
clean: CLEAN(CXX_BINARY(nvt-tcon-fw-updater))
all: CXX_BINARY(nvt-tcon-fw-updater)
