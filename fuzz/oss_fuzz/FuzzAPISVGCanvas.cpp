/*
 * Copyright 2020 Google, LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "fuzz/Fuzz.h"

void fuzz_SVGCanvas(Fuzz* f);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    auto fuzz = Fuzz(SkData::MakeWithoutCopy(data, size));
    fuzz_SVGCanvas(&fuzz);
    return 0;
}
