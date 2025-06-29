#ifndef TEXT_TO_SPEECH_H
#define TEXT_TO_SPEECH_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the text-to-speech system
 */
void text_to_speech_init(void);

/**
 * @brief Play text as speech
 * @param text The text to convert to speech
 */
void text_to_speech_play(const char *text);

/**
 * @brief Loop function to maintain audio processing
 * Must be called regularly in main loop
 */
void text_to_speech_loop(void);

/**
 * @brief Check if TTS is currently playing
 * @return true if TTS is playing, false otherwise
 */
bool text_to_speech_is_playing(void);

/**
 * @brief Stop current TTS playback
 */
void text_to_speech_stop(void);

#ifdef __cplusplus
}
#endif

#endif // TEXT_TO_SPEECH_H