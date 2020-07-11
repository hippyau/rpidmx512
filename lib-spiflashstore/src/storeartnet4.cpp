/**
 * @file storeartnet4.cpp
 *
 */
/* Copyright (C) 2019-2020 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <cassert>

#include "storeartnet4.h"

#include "artnet4params.h"

#include "spiflashstore.h"


#include "debug.h"

StoreArtNet4 *StoreArtNet4::s_pThis = nullptr;

StoreArtNet4::StoreArtNet4() {
	DEBUG_ENTRY

	assert(s_pThis == nullptr);
	s_pThis = this;

	DEBUG_PRINTF("%p", reinterpret_cast<void *>(s_pThis));
	DEBUG_EXIT
}

void StoreArtNet4::Update(const struct TArtNet4Params* pArtNet4Params) {
	DEBUG_ENTRY

	SpiFlashStore::Get()->Update(STORE_ARTNET4, pArtNet4Params, sizeof(struct TArtNet4Params));

	DEBUG_EXIT
}

void StoreArtNet4::Copy(struct TArtNet4Params* pArtNet4Params) {
	DEBUG_ENTRY

	SpiFlashStore::Get()->Copy(STORE_ARTNET4, pArtNet4Params, sizeof(struct TArtNet4Params));

	DEBUG_EXIT
}
