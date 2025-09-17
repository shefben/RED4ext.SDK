
#pragma once  
// Minimal opus fallback for testing
typedef struct OpusEncoder OpusEncoder;
inline OpusEncoder* opus_encoder_create(int Fs, int channels, int application, int *error) { return nullptr; }
