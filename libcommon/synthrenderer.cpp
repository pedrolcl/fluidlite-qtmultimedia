/*
    Sonivox EAS Synthesizer for Qt applications
    Copyright (C) 2016-2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

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

#include <QObject>
#include <QDebug>
#include <QString>
#include <QCoreApplication>
#include <QTextStream>
#include <drumstick/sequencererror.h>
#include "programsettings.h"
#include "synthrenderer.h"
#include "programsettings.h"

using namespace drumstick::rt;

SynthRenderer::SynthRenderer(QObject *parent):
    QIODevice(parent),
    m_input(nullptr),
    m_lastBufferSize(0)
{
    //qDebug() << Q_FUNC_INFO;
    initMIDI();
    initSynth();
}

void
SynthRenderer::initMIDI()
{
    //qDebug() << Q_FUNC_INFO << ProgramSettings::DEFAULT_MIDI_DRIVER;
    if (m_midiDriver.isEmpty()) {
        setMidiDriver(ProgramSettings::DEFAULT_MIDI_DRIVER);
    }
    if (m_input == nullptr) {
        qWarning() << Q_FUNC_INFO << "Input Backend is Missing. You may need to set the DRUMSTICKRT environment variable";    
    }
}

const int SynthRenderer::DEFAULT_SAMPLE_RATE = 44100;
const int SynthRenderer::DEFAULT_RENDERING_FRAMES = 64;
const int SynthRenderer::DEFAULT_FRAME_CHANNELS = 2;

void
SynthRenderer::initSynth()
{
    m_sampleRate = DEFAULT_SAMPLE_RATE;
    m_renderingFrames = DEFAULT_RENDERING_FRAMES;
    m_channels = DEFAULT_FRAME_CHANNELS;
    m_sample_size = sizeof(float) * CHAR_BIT;

    /* FluidLite initialization */
    m_settings = new_fluid_settings();
    fluid_settings_setnum(m_settings, "synth.sample-rate", m_sampleRate);
    fluid_settings_setnum(m_settings, "synth.gain", 1.0);
    m_synth = new_fluid_synth(m_settings);
    qDebug() << Q_FUNC_INFO << "synthesis frames:" << m_renderingFrames << "sample rate:" << m_sampleRate << "audio channels:" << m_channels;

    /* QAudioFormat initialization */
    m_format.setSampleRate(m_sampleRate);
    m_format.setChannelCount(m_channels);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_format.setSampleSize(m_sample_size);
    m_format.setCodec("audio/pcm");
    m_format.setSampleType(QAudioFormat::Float);
    m_format.setByteOrder(QAudioFormat::LittleEndian);
#else
    m_format.setSampleFormat(QAudioFormat::Float);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
#endif
}

SynthRenderer::~SynthRenderer()
{
    if (m_input != nullptr) {
        m_input->disconnect();
        m_input->close();
    }
    delete_fluid_synth(m_synth);
    delete_fluid_settings(m_settings);
    //qDebug() << Q_FUNC_INFO;
}

qint64 SynthRenderer::readData(char *data, qint64 maxlen)
{
    //qDebug() << Q_FUNC_INFO << "starting with maxlen:" << maxlen;
    const qint64 bufferSamples = m_renderingFrames * m_channels;
    const qint64 bufferBytes = bufferSamples * sizeof(float);
    Q_ASSERT(bufferBytes > 0 && bufferBytes <= maxlen);
    qint64 buflen = (maxlen / bufferBytes) * bufferBytes;
    qint64 length = buflen;
    
    float *buffer = reinterpret_cast<float *>(data);
    while (length > 0) {
        fluid_synth_write_float(m_synth, m_renderingFrames, buffer, 0, m_channels, buffer, 1, m_channels);
        length -= bufferBytes;
        buffer += bufferSamples;
    }

    m_lastBufferSize = buflen;
    //qDebug() << Q_FUNC_INFO << "before returning" << buflen;
    return buflen;
}

qint64 SynthRenderer::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    //qDebug() << Q_FUNC_INFO;
	return 0;
}

qint64 SynthRenderer::size() const
{
    //qDebug() << Q_FUNC_INFO;
    return std::numeric_limits<qint64>::max();
}

qint64 SynthRenderer::bytesAvailable() const
{
    //qDebug() << Q_FUNC_INFO;
    return std::numeric_limits<qint64>::max();
}

bool
SynthRenderer::stopped()
{
    //qDebug() << Q_FUNC_INFO;
    return !isOpen();
}

void
SynthRenderer::start()
{
    //qDebug() << Q_FUNC_INFO;
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void
SynthRenderer::stop()
{
    //qDebug() << Q_FUNC_INFO;
    if (isOpen()) {
        close();
    }
}

QStringList 
SynthRenderer::connections() const
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(m_input != nullptr);
    QStringList result;
    auto avail = m_input->connections(true);
    foreach(const auto &c, avail) {
        result << c.first;
    }
    return result;
}

QString 
SynthRenderer::subscription() const
{
    //qDebug() << Q_FUNC_INFO << m_portName;
    return m_portName;
}

void
SynthRenderer::subscribe(const QString& portName)
{
    if (portName != m_portName) {
        //qDebug() << Q_FUNC_INFO << portName;
        Q_ASSERT(m_input != nullptr);
        auto avail = m_input->connections(true);
        auto it = std::find_if(avail.constBegin(), avail.constEnd(),
                               [portName](const MIDIConnection& c) { 
                                   return c.first == portName; 
                               });
        m_input->close();
        if (it == avail.constEnd()) {
            MIDIConnection conn;
            m_input->open(conn);
            m_portName = conn.first;
        } else {
            m_input->open(*it);
            m_portName = (*it).first;
        }
    }
}

const QString 
SynthRenderer::midiDriver() const
{
    //qDebug() << Q_FUNC_INFO << m_midiDriver;
    return m_midiDriver;
}

void 
SynthRenderer::setMidiDriver(const QString newMidiDriver)
{
    if (m_midiDriver != newMidiDriver) {
        //qDebug() << Q_FUNC_INFO << newMidiDriver;
        m_midiDriver = newMidiDriver;
        if (m_input != nullptr) {
            m_input->disconnect();
            m_input->close();
        }
        m_input = m_man.inputBackendByName(m_midiDriver);
        if (m_input != nullptr) {
            QObject::connect(m_input, &MIDIInput::midiNoteOn, this, &SynthRenderer::noteOn);
            QObject::connect(m_input, &MIDIInput::midiNoteOff, this, &SynthRenderer::noteOff);
            QObject::connect(m_input, &MIDIInput::midiKeyPressure, this, &SynthRenderer::keyPressure);
            QObject::connect(m_input, &MIDIInput::midiController, this, &SynthRenderer::controller);
            QObject::connect(m_input, &MIDIInput::midiProgram, this, &SynthRenderer::program);
            QObject::connect(m_input, &MIDIInput::midiChannelPressure, this, &SynthRenderer::channelPressure);
            QObject::connect(m_input, &MIDIInput::midiPitchBend, this, &SynthRenderer::pitchBend);
        }
    }
}

void SynthRenderer::noteOn(const int chan, const int note, const int vel)
{
    //qDebug() << Q_FUNC_INFO << chan << note << vel;
    fluid_synth_noteon(m_synth, chan, note, vel);
    emit midiNoteOn(note, vel);
}

void SynthRenderer::noteOff(const int chan, const int note, const int vel)
{
    //qDebug() << Q_FUNC_INFO << chan << note;
    fluid_synth_noteoff(m_synth, chan, note);
    emit midiNoteOff(note, vel);
}

void SynthRenderer::keyPressure(const int chan, const int note, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << note << value;
    fluid_synth_key_pressure(m_synth, chan, note, value);
}

void SynthRenderer::controller(const int chan, const int control, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << control << value;
    fluid_synth_cc(m_synth, chan, control, value);
}

void SynthRenderer::program(const int chan, const int program) 
{
    //qDebug() << Q_FUNC_INFO << chan << program;
    fluid_synth_program_change(m_synth, chan, program);
}

void SynthRenderer::channelPressure(const int chan, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << value;
    fluid_synth_channel_pressure(m_synth, chan, value);
}

void SynthRenderer::pitchBend(const int chan, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << value;
    fluid_synth_pitch_bend(m_synth, chan, value);
}

void
SynthRenderer::initReverb(int reverb_type)
{
    //qDebug() << Q_FUNC_INFO << reverb_type;
    switch( reverb_type ) {
    case 1:
        fluid_synth_set_reverb(m_synth, 0.2, 0.2, 0.75, 0.8);
        break;
    case 2:
        fluid_synth_set_reverb(m_synth, 0.4, 0.2, 0.75, 0.8);
        break;
    case 3:
        fluid_synth_set_reverb(m_synth, 0.6, 0.2, 0.75, 0.8);
        break;
    case 4:
        fluid_synth_set_reverb(m_synth, 0.8, 0.2, 0.75, 0.8);
        break;
    case 5:
        fluid_synth_set_reverb(m_synth, 1.0, 0.2, 0.75, 0.8);
        break;
    };
    fluid_synth_set_reverb_on(m_synth, reverb_type > 0 ?  1 : 0 );
}

void
SynthRenderer::initChorus(int chorus_type)
{
    //qDebug() << Q_FUNC_INFO << chorus_type;
    fluid_synth_set_chorus_on(m_synth, chorus_type > 0 ?  1 : 0 );
}

void
SynthRenderer::setReverbLevel(int amount)
{
    qreal newlevel = amount / 100.0;
    qreal level = fluid_synth_get_reverb_level(m_synth);
    //qDebug() << Q_FUNC_INFO << level << newlevel;
    if (newlevel != level) {
        qreal roomsize = fluid_synth_get_reverb_roomsize(m_synth); 
        qreal damping = fluid_synth_get_reverb_damp(m_synth);
        qreal width = fluid_synth_get_reverb_width(m_synth); 
        fluid_synth_set_reverb(m_synth, roomsize, damping, width, newlevel);
    }
}

void
SynthRenderer::setChorusLevel(int amount)
{
    qreal newlevel = amount / 100.0;
    qreal level = fluid_synth_get_chorus_level(m_synth);
    //qDebug() << Q_FUNC_INFO << level << newlevel;
    if (newlevel != level) {
        int nr = fluid_synth_get_chorus_nr(m_synth);
        qreal speed = fluid_synth_get_chorus_speed_Hz(m_synth);
        qreal depth = fluid_synth_get_chorus_depth_ms(m_synth);
        int type = fluid_synth_get_chorus_type(m_synth);
        fluid_synth_set_chorus(m_synth, nr, newlevel, speed, depth, type);
    }
}

void
SynthRenderer::openSoundfont(const QString fileName)
{
    //qDebug() << Q_FUNC_INFO << fileName;
    m_file = fileName;
    if (m_synth != nullptr) {
        auto result = fluid_synth_sfload(m_synth, fileName.toLocal8Bit(), 1);
        m_sf2loaded = result != -1;
    }
}

qint64 SynthRenderer::lastBufferSize() const
{
    return m_lastBufferSize;
}

void SynthRenderer::resetLastBufferSize()
{
    m_lastBufferSize = 0;
}

const QAudioFormat&
SynthRenderer::format() const
{
    return m_format;
}
