#pragma once
#include "core/basic_types.h"

void audioInit();
bool audioWorks();
void audioPlayNotify();
void audioFinish();

enum AudioPlayerState {
	AudioPlayerStopped        = 0x01,
	AudioPlayerStoppedAtEnd   = 0x02,
	AudioPlayerStoppedAtError = 0x03,
	AudioPlayerStoppedAtStart = 0x04,
	AudioPlayerStoppedMask    = 0x07,

	AudioPlayerStarting       = 0x08,
	AudioPlayerPlaying        = 0x10,
	AudioPlayerFinishing      = 0x18,
	AudioPlayerPausing        = 0x20,
	AudioPlayerPaused         = 0x28,
	AudioPlayerPausedAtEnd    = 0x30,
	AudioPlayerResuming       = 0x38,
};

class AudioPlayer : public QObject {
	Q_OBJECT

public:

	AudioPlayer() {};
	void clearStoppedAtStart(const AudioMsgId &audio) { }
	void clearStoppedAtStart(SongMsgId const&) { }
	void currentState(AudioMsgId *audio, AudioPlayerState *state = 0, int64 *position = 0, int64 *duration = 0, int32 *frequency = 0) { };
	void currentState(SongMsgId *song, AudioPlayerState *state = 0, int64 *position = 0, int64 *duration = 0, int32 *frequency = 0) { };

	void pauseresume(MediaOverviewType type, bool fast = false) { };

	void play(const AudioMsgId &audio, int64 position = 0) { };
	void play(const SongMsgId &song, int64 position = 0) { };
	void stop(MediaOverviewType type) { };

	void stopAndClear() { };
	void seek(int64 position) { };

signals:
	void songVolumeChanged();

};

class AudioCapture : public QObject {
	Q_OBJECT
public:
	AudioCapture() { };

	void start() { };
	void stop(bool needResult) { };
};

MTPDocumentAttribute audioReadSongAttributes(const QString &fname, const QByteArray &data, QImage &cover, QByteArray &coverBytes, QByteArray &coverFormat);
VoiceWaveform audioCountWaveform(const FileLocation &file, const QByteArray &data);

AudioPlayer *audioPlayer();
AudioCapture *audioCapture();
