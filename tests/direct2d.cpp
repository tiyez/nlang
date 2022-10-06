



#include "direct2d.hpp"
#include <d2d1.h>
#include <dwrite.h>
#include <iostream>

struct direct2d_state {
	ID2D1Factory	*d2dfactory;
	IDWriteFactory	*dwrite_factory;
	IDWriteTextFormat	*text_format;
};

direct2d_state	*initialize_direct2d (void) {
	direct2d_state	*state;
    static const WCHAR msc_fontName[] = L"Verdana";
    static const FLOAT msc_fontSize = 50;
    HRESULT hr;

	state = (direct2d_state *) malloc (sizeof (direct2d_state));
	memset (state, 0, sizeof *state);

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &state->d2dfactory);

    if (SUCCEEDED (hr)) {
        // Create a DirectWrite factory.
        hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (state->dwrite_factory), reinterpret_cast<IUnknown **> (&state->dwrite_factory));
    }
    if (SUCCEEDED (hr)) {
        // Create a DirectWrite text format object.
        hr = state->dwrite_factory->CreateTextFormat(
            msc_fontName,
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            msc_fontSize,
            L"", //locale
            &state->text_format
            );
    }
    if (SUCCEEDED(hr)) {
        // Center the text horizontally and vertically.
        state->text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

        state->text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    if (!SUCCEEDED(hr)) {
    	free (state);
    	state = 0;
    }
	return state;
}

int		run_direct2d (direct2d_state *state) {
	std::cout << "Hello World" << std::endl;
	return 1;
}





