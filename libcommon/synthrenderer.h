/*
    FluidLite Synthesizer for Qt applications
    Copyright (C) 2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SYNTHRENDERER_H_
#define SYNTHRENDERER_H_

#include <QObject>
#include <QIODevice>
#include <QScopedPointer>
#include <QAudioFormat>
#include <drumstick/backendmanager.h>
#include <drumstick/rtmidiinput.h>
#include <fluidlite.h>

class SynthRenderer : public QIODevice
{
    Q_OBJECT

public:
    explicit SynthRenderer(QObject *parent = 0);
    virtual ~SynthRenderer();

    /* QIODevice */
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
	qint64 size() const override;
	qint64 bytesAvailable() const override;

    /* Drumstick::RT */
    const QString midiDriver() const;
    void setMidiDriver(const QString newMidiDriver);
    QStringList connections() const;
    QString subscription() const;
    void subscribe(const QString& portName);
    void start();
    void stop();
    bool stopped();

    /* FluidLite */
    void initReverb(int reverb_type);
    void initChorus(int chorus_type);
    void setReverbLevel(int amount);
    void setChorusLevel(int amount);
    void openSoundfont(const QString fileName);

    static const int DEFAULT_SAMPLE_RATE;
    static const int DEFAULT_RENDERING_FRAMES;
    static const int DEFAULT_FRAME_CHANNELS;

    /* Qt Multimedia */
    const QAudioFormat &format() const;
    qint64 lastBufferSize() const;
    void resetLastBufferSize();

signals:
    void midiNoteOn(const int note, const int vel);
    void midiNoteOff(const int note, const int vel);

public slots:
    void noteOn(const int chan, const int note, const int vel);
    void noteOff(const int chan, const int note, const int vel);
    void keyPressure(const int chan, const int note, const int value);
    void controller(const int chan, const int control, const int value);
    void program(const int chan, const int program);
    void channelPressure(const int chan, const int value);
    void pitchBend(const int chan, const int value);

private:
    void initMIDI();
    void initSynth();

private:
    /* Drumstick RT*/
    QString m_midiDriver;
    QString m_portName;
    drumstick::rt::BackendManager m_man;
    drumstick::rt::MIDIInput *m_input;

    /* FluidLite */
    int m_sampleRate, m_renderingFrames, m_channels, m_sample_size;
    fluid_settings_t *m_settings;
    fluid_synth_t *m_synth;
    bool m_sf2loaded;
    QString m_file;
    
    /* Qt Multimedia */
    int m_lastBufferSize;
    QAudioFormat m_format;
};

#endif /*SYNTHRENDERER_H_*/
