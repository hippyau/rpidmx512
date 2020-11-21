/**
 * @file rgbpanel.h
 *
 */
/* Copyright (C) 2020 by Arjan van Vught mailto:info@orangepi-dmx.nl
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
/**
 * PoC
 */

#ifndef RGBPANEL_H_
#define RGBPANEL_H_

#include <stdint.h>

#include "rgbpanelconst.h"

#include "../../lib-device/src/font_cp437.h"
#include "../../lib-device/src/font_5x8.h"


namespace rgbpanel {
static constexpr auto PWM_WIDTH = 84;
enum class TFontID { FONT_8x8, FONT_5x8 };
}  // namespace rgbpanel

class RgbPanel {
public:
	RgbPanel(uint32_t nColumns, uint32_t nRows, uint32_t nChain = rgbpanel::defaults::CHAIN, RgbPanelTypes type = rgbpanel::defaults::TYPE);
	~RgbPanel() {
		PlatformCleanUp();
	}

	void Start();
	void Stop();

	void SetPixel(uint32_t nColumn, uint32_t nRow, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void Cls();
	void Show();

	uint32_t GetShowCounter();
	uint32_t GetUpdatesCounter();

	void Dump();

	// Text
	void PutChar(char nChar, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void DrawChar(char nChar, uint8_t nX, uint8_t nY, rgbpanel::TFontID tFont, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void PutString(const char *pString, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void Text(const char *pText, uint8_t nLength, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void TextLine(uint8_t nLine, const char *pText, uint8_t nLength, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void SetCursorPos(uint8_t nCol, uint8_t nRow);
	void ClearLine(uint8_t nLine);
	void SetColon(uint8_t nChar, uint8_t nCol, uint8_t nRow, uint8_t nRed, uint8_t nGreen, uint8_t nBlue);
	void SetColonsOff();

	uint32_t GetMaxPosition() {
		return m_nMaxPosition;
	}

	uint32_t GetMaxLine() {
		return m_nMaxLine;
	}

	void SetFont(uint8_t nLine, rgbpanel::TFontID font_id);

	rgbpanel::TFontID GetFontID() {
		return m_TFontID;
	}

	void Print();

	static uint32_t ValidateColumns(uint32_t nColumns);
	static uint32_t ValidateRows(uint32_t nRows);
	static RgbPanelTypes GetType(const char *pType);
	static const char *GetType(RgbPanelTypes tType);

private:
	void PlatformInit();
	void PlatformCleanUp();

protected:
	uint32_t m_nColumns;
	uint32_t m_nRows;

private:	
	uint32_t m_nChain;
	RgbPanelTypes m_tType;
	bool m_bIsStarted{false};
	// Text
	uint32_t m_nMaxPosition;
	uint32_t m_nMaxLine;
	uint32_t m_nPosition{0};
	rgbpanel::TFontID m_LineFont[4];
	uint8_t m_LineStartX[4] = {0}; // offsets text line X


	uint32_t m_nLine{0};
	struct TColon {
		uint8_t nBits;
		uint8_t nRed;
		uint8_t nGreen;
		uint8_t nBlue;
	};
	TColon *m_ptColons{nullptr};
	rgbpanel::TFontID m_TFontID;

};

#endif /* RGBPANEL_H_ */
