/*
    Sonivox EAS Synthesizer for Qt applications
    Copyright (C) 2016-2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTextStream>
#include <QtDebug>
#include <QReadLocker>
#include <QWriteLocker>
#include <drumstick/sequencererror.h>
#include "synthrenderer.h"
#include "programsettings.h"

using namespace drumstick::rt;

SynthRenderer::SynthRenderer(int bufTime, QObject *parent) : QObject(parent),
    m_Stopped(true),
    m_input(nullptr),
    m_sf2loaded(false), 
	m_requestedBufferTime(bufTime)
{
    qDebug() << Q_FUNC_INFO << bufTime;
    initMIDI();
    initSynth();
    initAudioDevices();
}

void
SynthRenderer::initMIDI()
{
    const QString DEFAULT_DRIVER = 
#if defined(Q_OS_LINUX)
        QStringLiteral("ALSA");
#elif defined(Q_OS_WINDOWS)
        QStringLiteral("Windows MM");
#elif defined(Q_OS_MACOS)
        QStringLiteral("CoreMIDI");
#elif defined(Q_OS_UNIX)
        QStringLiteral("OSS");
#else
        QStringLiteral("Network");
#endif
    qDebug() << Q_FUNC_INFO << DEFAULT_DRIVER;
    if (m_midiDriver.isEmpty()) {
        setMidiDriver(DEFAULT_DRIVER);
    }
    if (m_input == nullptr) {
        qWarning() << "Input Backend is Missing. You may need to set the DRUMSTICKRT environment variable";
    }
}

void
SynthRenderer::initSynth()
{
    m_sampleRate = 44100;
    m_bufferSize = 64;
    m_channels = 2;
    m_sample_size = sizeof(float) * CHAR_BIT;
   
    /* FluidLite initialization */
    m_settings = new_fluid_settings();
    fluid_settings_setnum(m_settings, "synth.gain", 1.2);
    m_synth = new_fluid_synth(m_settings);
    qDebug() << Q_FUNC_INFO << "bufferSize=" << m_bufferSize << " sampleRate=" << m_sampleRate << " channels=" << m_channels;
}

void
SynthRenderer::initAudio()
{
    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    format.setSampleSize(m_sample_size);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::Float);
#else
    format.setSampleFormat(QAudioFormat::Float);
#endif

    if (!m_audioDevice.isFormatSupported(format)) {
        qCritical() << "Audio format not supported" << format;
        return;
    }

    qint64 requested_size = m_channels * (m_sample_size / CHAR_BIT) * m_requestedBufferTime * m_sampleRate / 1000;
    qint64 period_bytes = m_channels * (m_sample_size / CHAR_BIT) * m_bufferSize;
    qDebug() << Q_FUNC_INFO << "requested buffer sizes:" << period_bytes << requested_size;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_audioOutput.reset(new QAudioOutput(m_audioDevice, format));
    m_audioOutput->setCategory("MIDI Synthesizer");
    QObject::connect(m_audioOutput.data(), &QAudioOutput::stateChanged, this, [](QAudio::State state){
#else
    m_audioOutput.reset(new QAudioSink(m_audioDevice, format));
    QObject::connect(m_audioOutput.data(), &QAudioSink::stateChanged, this, [](QAudio::State state){
#endif
        qDebug() << "Audio Output state changed:" << state;
    });
    m_audioOutput->setBufferSize( qMax(period_bytes, requested_size) );
}

void SynthRenderer::initAudioDevices()
{
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_availableDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    m_audioDevice = QAudioDeviceInfo::defaultOutputDevice();
#else
    QMediaDevices devices;
    m_availableDevices = devices.audioOutputs();
    m_audioDevice = devices.defaultAudioOutput();
#endif
    /*foreach(auto &dev, m_availableDevices) {
        qDebug() << Q_FUNC_INFO << dev.deviceName();
    }*/
}

SynthRenderer::~SynthRenderer()
{
    if (m_input != nullptr) {
        m_input->close();
    }
    delete_fluid_synth(m_synth);
    delete_fluid_settings(m_settings);
    qDebug() << Q_FUNC_INFO;
}

bool
SynthRenderer::stopped()
{
	QReadLocker locker(&m_mutex);
    return m_Stopped;
}

void
SynthRenderer::stop()
{
	QWriteLocker locker(&m_mutex);
    qDebug() << Q_FUNC_INFO;
    m_Stopped = true;
}

QStringList 
SynthRenderer::connections() const
{
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
    return m_portName;
}

void
SynthRenderer::subscribe(const QString& portName)
{
    qDebug() << Q_FUNC_INFO << portName;
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
    } else {
        m_input->open(*it);
    }
}

void
SynthRenderer::run()
{
    QByteArray audioData;
    initAudio();
    qDebug() << Q_FUNC_INFO << "started";
    try {
        if (m_input != nullptr) {
            m_input->disconnect();
            QObject::connect(m_input, &MIDIInput::midiNoteOn, this, &SynthRenderer::noteOn);
            QObject::connect(m_input, &MIDIInput::midiNoteOff, this, &SynthRenderer::noteOff);
            QObject::connect(m_input, &MIDIInput::midiKeyPressure, this, &SynthRenderer::keyPressure);
            QObject::connect(m_input, &MIDIInput::midiController, this, &SynthRenderer::controller);
            QObject::connect(m_input, &MIDIInput::midiProgram, this, &SynthRenderer::program);
            QObject::connect(m_input, &MIDIInput::midiChannelPressure, this, &SynthRenderer::channelPressure);
            QObject::connect(m_input, &MIDIInput::midiPitchBend, this, &SynthRenderer::pitchBend);
        }
        if (m_synth != nullptr && !m_sf2loaded) {
            openSoundfont(ProgramSettings::instance()->soundFontFile());
        }
        m_Stopped = false;
        QIODevice *iodevice = m_audioOutput->start();
        qDebug() << "Audio Output started with buffer size =" << m_audioOutput->bufferSize() << "bytesfree= " << m_audioOutput->bytesFree();
        audioData.reserve(m_audioOutput->bufferSize());
        while (!stopped()) {
            QCoreApplication::sendPostedEvents();
            if (m_audioOutput->state() == QAudio::SuspendedState || 
                m_audioOutput->state() == QAudio::StoppedState) {
                qDebug() << Q_FUNC_INFO << "leaving";
                break;
            }
            if (m_synth != 0)
            {
                // synth audio rendering
                int maxlen = m_audioOutput->bufferSize();
                while(audioData.size() < maxlen) {
                    int bytes = m_bufferSize * sizeof(float) * m_channels;
                    char buffer[bytes];
                    fluid_synth_write_float(m_synth, m_bufferSize, buffer, 0, m_channels, buffer, 1, m_channels);
                    audioData.append(buffer, bytes);
                }
                // hand over to audiooutput, pushing the rendered buffer
                maxlen = qMin(maxlen, m_audioOutput->bytesFree());
                int written = iodevice->write(audioData, maxlen);
                if (written < 0 || m_audioOutput->error() != QAudio::NoError) {
                    qWarning() << Q_FUNC_INFO << "write audio error:" << m_audioOutput->error();
                    break;
                } else if (written > 0) {
                    //qDebug() << Q_FUNC_INFO << written;
                    audioData.remove(0, written);
                }
            }
        }
        m_audioOutput->stop();
        qDebug() << "QAudioOutput stopped";
    } catch (...) {
        qWarning() << "Error! exception";
    }
    qDebug() << Q_FUNC_INFO << "ended";
    emit finished();
}

const QString SynthRenderer::midiDriver() const
{
    return m_midiDriver;
}

void SynthRenderer::setMidiDriver(const QString newMidiDriver)
{
    if (m_midiDriver != newMidiDriver) {
        m_midiDriver = newMidiDriver;
        if (m_input != nullptr) {
            m_input->close();
        }
        m_input = m_man.inputBackendByName(m_midiDriver);
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
const QAudioDeviceInfo &SynthRenderer::audioDevice() const
{
    return m_audioDevice;
}

void SynthRenderer::setAudioDevice(const QAudioDeviceInfo &newAudioDevice)
{
    m_audioDevice = newAudioDevice;
}
#else
const QAudioDevice &SynthRenderer::audioDevice() const
{
    return m_audioDevice;
}

void SynthRenderer::setAudioDevice(const QAudioDevice &newAudioDevice)
{
    m_audioDevice = newAudioDevice;
}
#endif


QStringList SynthRenderer::availableAudioDevices() const
{
    QStringList result;
    foreach(const auto &device, m_availableDevices) {
    #if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        result << device.deviceName();
    #else
        result << device.description();
    #endif
    }
    return result;
}

QString SynthRenderer::audioDeviceName() const
{
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    return m_audioDevice.deviceName();
#else
    return m_audioDevice.description();
#endif
}

void SynthRenderer::setAudioDeviceName(const QString newName)
{
    foreach(auto device, m_availableDevices) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        if (device.deviceName() == newName) {
#else
        if (device.description() == newName) {
#endif
            m_audioDevice = device;
            break;
        }
    }
}

void SynthRenderer::noteOn(int chan, int note, int vel) 
{
    //qDebug() << Q_FUNC_INFO << chan << note << vel;
    fluid_synth_noteon(m_synth, chan, note, vel);
}

void SynthRenderer::noteOff(int chan, int note, int vel) 
{
    Q_UNUSED(vel)
    //qDebug() << Q_FUNC_INFO << chan << note;
    fluid_synth_noteoff(m_synth, chan, note);
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
    Q_UNUSED(reverb_type)
}

void
SynthRenderer::initChorus(int chorus_type)
{
    Q_UNUSED(chorus_type)
}

void
SynthRenderer::setReverbWet(int amount)
{
    Q_UNUSED(amount)
}

void
SynthRenderer::setChorusLevel(int amount)
{
    Q_UNUSED(amount)
}

void
SynthRenderer::openSoundfont(const QString fileName)
{
    qDebug() << Q_FUNC_INFO << fileName;
    m_file = fileName;
    if (m_synth != nullptr) {
        auto result = fluid_synth_sfload(m_synth, fileName.toLocal8Bit(), 1);
        m_sf2loaded = result != -1;
    }
}
