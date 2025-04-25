#pragma once

// Token types
typedef enum {
	SDK_MELODY_TOKEN_NOTE,	      // Musical note with optional sharp and length
	SDK_MELODY_TOKEN_REST,	      // _ for rests
	SDK_MELODY_TOKEN_OCTAVE_UP,   // > for octave up
	SDK_MELODY_TOKEN_OCTAVE_DOWN, // < for octave down
	SDK_MELODY_TOKEN_DYNAMIC,     // Dynamic markings like (mf), (pp), etc.
	SDK_MELODY_TOKEN_LOOP_START,  // { for loop start
	SDK_MELODY_TOKEN_LOOP_END,    // } for loop end
	SDK_MELODY_TOKEN_INSTRUMENT,  // /i:sine, /i:square, etc.
	SDK_MELODY_TOKEN_BPM,	      // /bpm:120, etc.
	SDK_MELODY_TOKEN_PANNING,     // Panning like /pc, /plll, /prrr
	SDK_MELODY_TOKEN_ERROR,	      // Invalid token
	SDK_MELODY_TOKEN_END	      // End of string
} sdk_melody_token_type_t;

typedef enum {
	SDK_MELODY_INSTRUMENT_SINE = 0, // Sine wave
	SDK_MELODY_INSTRUMENT_SQUARE,	// Square wave
	SDK_MELODY_INSTRUMENT_NOISE,	// White noise
	SDK_MELODY_INSTRUMENT_PHI,	// Phi-derived patterns
	SDK_MELODY_INSTRUMENT_PRNL,	// Pseudorandom noise loop
} sdk_melody_instrument_t;

// Tagged union for token data
typedef struct {
	sdk_melody_token_type_t type;
	union {
		struct {
			int note;
			int length;
		};
		sdk_melody_instrument_t instrument;
		int dynamic, bpm, panning;
	};
} sdk_melody_token_t;

// re2c tokenizer function
sdk_melody_token_t sdk_melody_lex(const char **input_ptr);
