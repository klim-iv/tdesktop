#include "stdafx.h"
#include "audio.h"

void audioInit() {}
bool audioWorks() { return false; }
void audioPlayNotify() {}
void audioFinish() {}

AudioPlayer *audioPlayer() {
	return nullptr;
}

AudioCapture *audioCapture() {
	return nullptr;
}

MTPDocumentAttribute audioReadSongAttributes(const QString &fname, const QByteArray &data, QImage &cover, QByteArray &coverBytes, QByteArray &coverFormat) {
	return MTP_documentAttributeFilename(MTP_string(fname));
}

VoiceWaveform audioCountWaveform(const FileLocation &file, const QByteArray &data) {
	return VoiceWaveform();
}
